#pragma once

#include "common.h"
#include <float.h>

/*
 * Measures the GPU time of a block of D3D11 commands using timestamp queries.
 * The results are read back a few frames later, so measuring never stalls the GPU.
 *
 * The block can be split into sections with Mark(): the time from one Mark() to the
 * next one (or to End()) is added to that section. The time before the first Mark()
 * goes to section 0. A section can be marked many times per frame, its times add up.
 *
 * Usage, once per frame:
 *   timer.Begin(device, context, tag);
 *   timer.Mark(context, SECTION_A);
 *   ... commands ...
 *   timer.Mark(context, SECTION_B);
 *   ... commands ...
 *   timer.End(context);
 *   if (timer.GetReport(5000, &stats, &tag)) { ... log stats ... }
 *
 * The tag identifies the code path being measured. Samples taken with different tags
 * are never averaged together: changing the tag starts a new reporting window.
 */
class GPUTimer
{
public:
	static const int MAX_SECTIONS = 8;

	struct Stats
	{
		double avgMs, minMs, maxMs;
		double sectionAvgMs[MAX_SECTIONS];
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
		s.numTimestamps = 0;
		context->Begin(s.disjoint);
		_measuring = true;
		AddTimestamp(context, 0);
	}

	// Starts a new section. Does nothing unless Begin() started a measurement.
	void Mark(ID3D11DeviceContext* context, int section)
	{
		// The last timestamp is reserved for End(). If there are too many marks, the
		// rest of the time is added to the current section.
		if (!_measuring || section < 0 || section >= MAX_SECTIONS ||
			_sets[_cur].numTimestamps >= MAX_TIMESTAMPS - 1)
			return;

		AddTimestamp(context, section);
	}

	void End(ID3D11DeviceContext* context)
	{
		if (!_measuring)
			return;

		QuerySet& s = _sets[_cur];
		// This timestamp only closes the last section, so its section doesn't matter
		AddTimestamp(context, 0);
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
		for (int i = 0; i < MAX_SECTIONS; i++)
			stats->sectionAvgMs[i] = _sectionSumMs[i] / _numFrames;
		stats->numFrames = _numFrames;
		*tag = _windowTag;
		ResetWindow(_windowTag);
		return true;
	}

private:
	static const int NUM_QUERY_SETS = 4;
	static const int MAX_TIMESTAMPS = 64;

	struct QuerySet
	{
		ComPtr<ID3D11Query> disjoint;
		ComPtr<ID3D11Query> timestamps[MAX_TIMESTAMPS];
		// sections[i] is the section that starts at timestamps[i]
		int sections[MAX_TIMESTAMPS];
		int numTimestamps = 0;
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
	double _sectionSumMs[MAX_SECTIONS] = {};
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
			s.numTimestamps = 0;
			if (FAILED(device->CreateQuery(&disjointDesc, &s.disjoint)))
			{
				log_debug("[DBG] [GPUTimer] Could not create the timestamp queries");
				return false;
			}
			for (int i = 0; i < MAX_TIMESTAMPS; i++)
			{
				if (FAILED(device->CreateQuery(&timestampDesc, &s.timestamps[i])))
				{
					log_debug("[DBG] [GPUTimer] Could not create the timestamp queries");
					return false;
				}
			}
		}

		_device = device;
		ResetWindow(_windowTag);
		return true;
	}

	void AddTimestamp(ID3D11DeviceContext* context, int section)
	{
		QuerySet& s = _sets[_cur];
		s.sections[s.numTimestamps] = section;
		context->End(s.timestamps[s.numTimestamps]);
		s.numTimestamps++;
	}

	// Reads back every query set that the GPU has finished with.
	void Collect(ID3D11DeviceContext* context)
	{
		const UINT flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;
		UINT64 t[MAX_TIMESTAMPS];
		for (QuerySet& s : _sets)
		{
			if (!s.pending)
				continue;

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
			if (context->GetData(s.disjoint, &disjoint, sizeof(disjoint), flags) != S_OK)
				continue;

			bool ready = true;
			for (int i = 0; i < s.numTimestamps && ready; i++)
				ready = context->GetData(s.timestamps[i], &t[i], sizeof(t[i]), flags) == S_OK;
			if (!ready)
				continue;

			s.pending = false;
			// Disjoint means the GPU clock changed during the measurement, so the
			// timestamps can't be trusted.
			if (disjoint.Disjoint || disjoint.Frequency == 0 || s.numTimestamps < 2 || s.tag != _windowTag)
				continue;

			const double toMs = 1000.0 / (double)disjoint.Frequency;
			double sectionMs[MAX_SECTIONS] = {};
			bool valid = true;
			for (int i = 0; i + 1 < s.numTimestamps; i++)
			{
				if (t[i + 1] < t[i])
				{
					valid = false;
					break;
				}
				sectionMs[s.sections[i]] += (double)(t[i + 1] - t[i]) * toMs;
			}
			if (!valid)
				continue;

			double ms = (double)(t[s.numTimestamps - 1] - t[0]) * toMs;
			_sumMs += ms;
			if (ms < _minMs) _minMs = ms;
			if (ms > _maxMs) _maxMs = ms;
			for (int i = 0; i < MAX_SECTIONS; i++)
				_sectionSumMs[i] += sectionMs[i];
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
		for (int i = 0; i < MAX_SECTIONS; i++)
			_sectionSumMs[i] = 0.0;
		_numFrames = 0;
	}
};
