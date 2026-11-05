#pragma once

#include "common.h"
#include <float.h>

/*
 * Measures the GPU time of a block of D3D11 commands using timestamp queries.
 * The results are read back a few frames later, so measuring never stalls the GPU.
 *
 * Usage, once per frame:
 *   timer.Begin(device, context, tag);
 *   ... commands to measure ...
 *   timer.End(context);
 *   if (timer.GetReport(5000, &stats, &tag)) { ... log stats ... }
 *
 * The tag identifies the code path being measured. Samples taken with different tags
 * are never averaged together: changing the tag starts a new reporting window.
 */
class GPUTimer
{
public:
	struct Stats
	{
		double avgMs, minMs, maxMs;
		int numFrames;
	};

	void Begin(ID3D11Device* device, ID3D11DeviceContext* context, int tag)
	{
		_measuring = false;
		if (device != _device && !CreateQueries(device))
			return;

		Collect(context);
		if (tag != _windowTag)
			ResetWindow(tag);

		QuerySet& s = _sets[_cur];
		// If this set is still in flight, the GPU is more than NUM_QUERY_SETS frames
		// behind. Skip this frame instead of waiting for it.
		if (s.pending)
			return;

		s.tag = tag;
		context->Begin(s.disjoint);
		context->End(s.start);
		_measuring = true;
	}

	void End(ID3D11DeviceContext* context)
	{
		if (!_measuring)
			return;

		QuerySet& s = _sets[_cur];
		context->End(s.end);
		context->End(s.disjoint);
		s.pending = true;
		_cur = (_cur + 1) % NUM_QUERY_SETS;
		_measuring = false;
	}

	// Returns true once windowMs milliseconds have elapsed since the last report and at
	// least one frame was measured. Fills stats and the tag they belong to, and starts
	// a new reporting window.
	bool GetReport(ULONGLONG windowMs, Stats* stats, int* tag)
	{
		if (_numFrames == 0 || GetTickCount64() - _windowStart < windowMs)
			return false;

		stats->avgMs = _sumMs / _numFrames;
		stats->minMs = _minMs;
		stats->maxMs = _maxMs;
		stats->numFrames = _numFrames;
		*tag = _windowTag;
		ResetWindow(_windowTag);
		return true;
	}

private:
	static const int NUM_QUERY_SETS = 4;

	struct QuerySet
	{
		ComPtr<ID3D11Query> disjoint, start, end;
		bool pending = false;
		int tag = 0;
	};

	QuerySet _sets[NUM_QUERY_SETS];
	ID3D11Device* _device = nullptr;
	int _cur = 0;
	bool _measuring = false;

	int _windowTag = -1;
	ULONGLONG _windowStart = 0;
	double _sumMs = 0.0, _minMs = DBL_MAX, _maxMs = 0.0;
	int _numFrames = 0;

	bool CreateQueries(ID3D11Device* device)
	{
		_device = nullptr;
		_cur = 0;
		_measuring = false;

		D3D11_QUERY_DESC disjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
		D3D11_QUERY_DESC timestampDesc = { D3D11_QUERY_TIMESTAMP, 0 };
		for (QuerySet& s : _sets)
		{
			s.pending = false;
			if (FAILED(device->CreateQuery(&disjointDesc, &s.disjoint)) ||
				FAILED(device->CreateQuery(&timestampDesc, &s.start)) ||
				FAILED(device->CreateQuery(&timestampDesc, &s.end)))
			{
				log_debug("[DBG] [GPUTimer] Could not create the timestamp queries");
				return false;
			}
		}

		_device = device;
		ResetWindow(_windowTag);
		return true;
	}

	// Reads back every query set that the GPU has finished with.
	void Collect(ID3D11DeviceContext* context)
	{
		const UINT flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;
		for (QuerySet& s : _sets)
		{
			if (!s.pending)
				continue;

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
			UINT64 t0, t1;
			if (context->GetData(s.disjoint, &disjoint, sizeof(disjoint), flags) != S_OK ||
				context->GetData(s.start, &t0, sizeof(t0), flags) != S_OK ||
				context->GetData(s.end, &t1, sizeof(t1), flags) != S_OK)
				continue;

			s.pending = false;
			// Disjoint means the GPU clock changed during the measurement, so the
			// timestamps can't be trusted.
			if (disjoint.Disjoint || disjoint.Frequency == 0 || t1 < t0 || s.tag != _windowTag)
				continue;

			double ms = (double)(t1 - t0) * 1000.0 / (double)disjoint.Frequency;
			_sumMs += ms;
			if (ms < _minMs) _minMs = ms;
			if (ms > _maxMs) _maxMs = ms;
			_numFrames++;
		}
	}

	void ResetWindow(int tag)
	{
		_windowTag = tag;
		_windowStart = GetTickCount64();
		_sumMs = 0.0;
		_minMs = DBL_MAX;
		_maxMs = 0.0;
		_numFrames = 0;
	}
};
