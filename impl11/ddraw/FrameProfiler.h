#pragma once

#include "common.h"
#include <float.h>
#include <climits>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * Frame profiler: the CPU and GPU time of every annotated event of the frame
 * (DeviceResources::BeginAnnotatedEvent() / EndAnnotatedEvent()), plus the frame as a whole.
 * EnableFrameProfiler in ddraw.cfg sets the starting mode, and Ctrl+Alt+Y cycles through them:
 * 0 off, 1 normal, 2 precise. It reports every 5 seconds to the debug output (DebugView),
 * never to a file.
 *
 * - CPU: QueryPerformanceCounter at the start and end of each event. "incl" is the event's
 *   whole time, "self" excludes its child events. The frame is measured from one Present() to
 *   the next; "outside events" is the time the CPU spent in the game and in code that isn't
 *   annotated, and Present() (VSync, or waiting for the GPU) is reported separately.
 * - GPU: timestamp queries at the start and end of each event, read back a few frames later,
 *   so it never stalls. "incl" and "self" work like the CPU's. Events that are still open at
 *   the frame boundary (Present) have no GPU time, but their children do.
 *
 * Normal mode (1): nothing waits, so the frame runs at its normal speed. But when the GPU is
 * the bottleneck, work queues up, and an event's timestamps can include the end of earlier
 * work (or a wait): GPU time moves from event to event between reports, while the frame
 * total stays right.
 *
 * Precise mode (2): at every event boundary, the CPU waits until the GPU is idle (an event
 * query) before the next timestamp. Every event's GPU work is measured alone, so the GPU
 * column is each event's real cost, and the sum of the GPU self times is the frame's GPU work:
 * the frame rate the GPU alone would allow. The waits make the frame slower, so the CPU
 * columns and the frame rate itself aren't representative in this mode.
 *
 * More probes: FRAME_PROFILE(L"name"); times the rest of the enclosing scope as an event.
 */
class FrameProfiler
{
public:
	// Requested mode (ddraw.cfg, Ctrl+Alt+Y): 0 off, 1 normal, 2 precise. Applied at the next
	// frame boundary, so that no event is cut in half.
	int requestedMode = 0;

	void BeginEvent(ID3D11Device* device, ID3D11DeviceContext* context, LPCWSTR name)
	{
		if (_mode == 0)
			return;
		if (_stack.size() >= MAX_DEPTH)
		{
			_overflow = true;
			return;
		}
		Open open;
		open.id = GetId(name, (int)_stack.size());
		open.childTicks = 0;
		open.gpuSet = -1;
		open.gpuSlot = -1;
		// GPU timestamps for the events that fit in this frame's query set. The slot is reserved
		// now, so that the children can point to their parent.
		QuerySet& s = _sets[_cur];
		if (device == _device && s.open && s.numTimestamps + 2 <= MAX_TIMESTAMPS)
		{
			if (_mode == 2)
				WaitForGpu(context);
			GpuEvent e;
			e.id = open.id;
			e.begin = AddTimestamp(context);
			e.end = -1;
			e.parent = ParentSlot();
			open.gpuSet = _cur;
			open.gpuSlot = (int)s.events.size();
			s.events.push_back(e);
		}
		_stack.push_back(open);
		_stack.back().t0 = Now();
	}

	void EndEvent(ID3D11DeviceContext* context)
	{
		if (_mode == 0 || _stack.empty())
			return;
		const LONGLONG t1 = Now();
		Open open = _stack.back();
		_stack.pop_back();
		const LONGLONG ticks = t1 - open.t0;
		Stats& st = _stats[open.id];
		st.calls++;
		st.cpuIncl += ticks;
		st.cpuSelf += ticks - open.childTicks;
		if (!_stack.empty())
			_stack.back().childTicks += ticks;
		else
			_topLevelTicks += ticks;
		// GPU: only when the event started and ended in the same query set
		if (open.gpuSet >= 0 && open.gpuSet == _cur && _sets[_cur].open)
		{
			QuerySet& s = _sets[_cur];
			if (s.numTimestamps < MAX_TIMESTAMPS)
			{
				if (_mode == 2)
					WaitForGpu(context);
				s.events[open.gpuSlot].end = AddTimestamp(context);
			}
		}
	}

	// Call before and after the frame's Present(): its time is reported as an event of its own,
	// and the frame ends after it
	void PresentBegin(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		BeginEvent(device, context, L"Present");
	}

	void PresentEnd(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		EndEvent(context);
		FrameBoundary(device, context);
	}

	// The device and context of the last frame, for the probes outside DeviceResources
	ID3D11Device* Device() { return _device; }
	ID3D11DeviceContext* Context() { return _context; }
	bool IsEnabled() const { return _mode != 0; }
	int Mode() const { return _mode; }

	// The frame rate and frame time for the screen (PrimarySurface::RenderText()), updated
	// twice a second; 0 until the first update
	double DisplayFps() const { return _displayFps; }
	double DisplayFrameMs() const { return _displayFrameMs; }

private:
	static const int NUM_QUERY_SETS = 5;
	static const int MAX_TIMESTAMPS = 512;
	static const size_t MAX_DEPTH = 64;
	static const ULONGLONG REPORT_MS = 5000;

	struct Stats
	{
		std::wstring name;
		int minDepth = 1000;
		int calls = 0;
		LONGLONG cpuIncl = 0, cpuSelf = 0;
		double gpuIncl = 0.0, gpuSelf = 0.0;
		int gpuCalls = 0;
	};

	struct Open
	{
		int id;
		LONGLONG t0;
		LONGLONG childTicks;
		int gpuSet;
		int gpuSlot;
	};

	struct GpuEvent
	{
		int id, begin, end; // end = -1: never closed in this query set
		int parent;         // The parent's slot in the same query set, or -1
	};

	struct QuerySet
	{
		ComPtr<ID3D11Query> disjoint;
		ComPtr<ID3D11Query> timestamps[MAX_TIMESTAMPS];
		int numTimestamps = 0;
		std::vector<GpuEvent> events;
		bool open = false;    // Taking timestamps (between two frame boundaries)
		bool pending = false; // Waiting for the GPU
		int window = 0;
	};

	int _mode = 0;
	bool _overflow = false;
	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _context = nullptr;
	QuerySet _sets[NUM_QUERY_SETS];
	ComPtr<ID3D11Query> _idleQuery; // Precise mode: tells when the GPU is idle
	int _cur = 0;

	std::vector<Open> _stack;
	std::vector<Stats> _stats; // In the order the events were first seen: the frame's order
	std::unordered_map<const wchar_t*, int> _idByPtr;
	std::unordered_map<std::wstring, int> _idByName;

	LARGE_INTEGER _freq = {};
	LONGLONG _lastBoundary = 0;
	LONGLONG _topLevelTicks = 0;
	int _window = 0;
	ULONGLONG _windowStart = 0;
	int _frames = 0;
	LONGLONG _frameTicksSum = 0, _frameTicksMin = LLONG_MAX, _frameTicksMax = 0;
	LONGLONG _topLevelSum = 0;
	double _gpuFrameMsSum = 0.0;
	double _gpuSelfMsSum = 0.0; // The sum of the events' GPU self times, over the window
	int _gpuFrames = 0;
	// For the screen: the frames since the last update, and their time
	int _displayFrames = 0;
	LONGLONG _displayTicks = 0;
	double _displayFps = 0.0, _displayFrameMs = 0.0;

	LONGLONG Now()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return t.QuadPart;
	}

	double TicksToMs(double ticks) const { return ticks * 1000.0 / (double)_freq.QuadPart; }

	// The slot of the innermost open event with a GPU slot in the current query set, or -1
	int ParentSlot() const
	{
		for (int i = (int)_stack.size() - 1; i >= 0; i--)
			if (_stack[i].gpuSet == _cur && _stack[i].gpuSlot >= 0)
				return _stack[i].gpuSlot;
		return -1;
	}

	// Precise mode: waits until the GPU has finished everything submitted so far
	void WaitForGpu(ID3D11DeviceContext* context)
	{
		if (_idleQuery == nullptr)
			return;
		context->End(_idleQuery);
		BOOL done = FALSE;
		// Without D3D11_ASYNC_GETDATA_DONOTFLUSH, GetData flushes the commands to the GPU
		while (context->GetData(_idleQuery, &done, sizeof(done), 0) != S_OK || !done)
			YieldProcessor();
	}

	int GetId(LPCWSTR name, int depth)
	{
		int id;
		auto it = _idByPtr.find(name);
		if (it != _idByPtr.end())
			id = it->second;
		else
		{
			// The same name may come from different string literals: merge them by content
			std::wstring s(name);
			auto it2 = _idByName.find(s);
			if (it2 != _idByName.end())
				id = it2->second;
			else
			{
				id = (int)_stats.size();
				Stats st;
				st.name = s;
				_stats.push_back(st);
				_idByName[s] = id;
			}
			_idByPtr[name] = id;
		}
		if (depth < _stats[id].minDepth)
			_stats[id].minDepth = depth;
		return id;
	}

	int AddTimestamp(ID3D11DeviceContext* context)
	{
		QuerySet& s = _sets[_cur];
		const int i = s.numTimestamps++;
		context->End(s.timestamps[i]);
		return i;
	}

	bool CreateQueries(ID3D11Device* device)
	{
		_device = nullptr;
		D3D11_QUERY_DESC disjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
		D3D11_QUERY_DESC timestampDesc = { D3D11_QUERY_TIMESTAMP, 0 };
		D3D11_QUERY_DESC eventDesc = { D3D11_QUERY_EVENT, 0 };
		for (QuerySet& s : _sets)
		{
			s.open = false;
			s.pending = false;
			s.numTimestamps = 0;
			s.events.clear();
			if (FAILED(device->CreateQuery(&disjointDesc, &s.disjoint)))
				return false;
			for (int i = 0; i < MAX_TIMESTAMPS; i++)
				if (FAILED(device->CreateQuery(&timestampDesc, &s.timestamps[i])))
					return false;
		}
		if (FAILED(device->CreateQuery(&eventDesc, &_idleQuery)))
			return false;
		_device = device;
		return true;
	}

	// Reads back the query sets that the GPU has finished with
	void Collect(ID3D11DeviceContext* context)
	{
		const UINT flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;
		std::vector<UINT64> t(MAX_TIMESTAMPS);
		std::vector<double> incl, childSum;
		for (QuerySet& s : _sets)
		{
			if (!s.pending)
				continue;
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
			if (context->GetData(s.disjoint, &disjoint, sizeof(disjoint), flags) != S_OK)
				continue;
			bool ready = true;
			for (int i = 0; i < s.numTimestamps && ready; i++)
				ready = context->GetData(s.timestamps[i], &t[i], sizeof(UINT64), flags) == S_OK;
			if (!ready)
				continue;
			s.pending = false;
			if (disjoint.Disjoint || disjoint.Frequency == 0 || s.numTimestamps < 2 || s.window != _window)
				continue;
			const double toMs = 1000.0 / (double)disjoint.Frequency;

			// Each event's time, then its self time: its time minus its children's
			const size_t n = s.events.size();
			incl.assign(n, -1.0);
			childSum.assign(n, 0.0);
			for (size_t i = 0; i < n; i++)
			{
				const GpuEvent& e = s.events[i];
				if (e.end < 0 || t[e.end] < t[e.begin])
					continue;
				incl[i] = (double)(t[e.end] - t[e.begin]) * toMs;
				if (e.parent >= 0)
					childSum[e.parent] += incl[i];
			}
			double frameSelf = 0.0;
			for (size_t i = 0; i < n; i++)
			{
				if (incl[i] < 0.0)
					continue;
				const double self = max(incl[i] - childSum[i], 0.0);
				Stats& st = _stats[s.events[i].id];
				st.gpuIncl += incl[i];
				st.gpuSelf += self;
				st.gpuCalls++;
				frameSelf += self;
			}
			// The frame's span on the GPU: from its first timestamp (the previous frame
			// boundary) to its last one (this one)
			if (t[s.numTimestamps - 1] >= t[0])
			{
				_gpuFrameMsSum += (double)(t[s.numTimestamps - 1] - t[0]) * toMs;
				_gpuSelfMsSum += frameSelf;
				_gpuFrames++;
			}
		}
	}

	void ResetWindow()
	{
		_window++;
		_windowStart = GetTickCount64();
		_frames = 0;
		_frameTicksSum = 0;
		_frameTicksMin = LLONG_MAX;
		_frameTicksMax = 0;
		_topLevelSum = 0;
		_gpuFrameMsSum = 0.0;
		_gpuSelfMsSum = 0.0;
		_gpuFrames = 0;
		for (Stats& st : _stats)
		{
			st.calls = 0;
			st.cpuIncl = st.cpuSelf = 0;
			st.gpuIncl = st.gpuSelf = 0.0;
			st.gpuCalls = 0;
		}
	}

	void FrameBoundary(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Off (EnableFrameProfiler = 0): nothing else is done, not even reading the clock
		if (_mode == 0 && requestedMode == 0)
			return;
		const LONGLONG now = Now();

		// Apply the requested mode
		if (requestedMode != _mode)
		{
			_mode = requestedMode;
			_stack.clear();
			_lastBoundary = 0;
			for (QuerySet& s : _sets)
			{
				s.open = false;
				s.pending = false;
			}
			if (_freq.QuadPart == 0)
				QueryPerformanceFrequency(&_freq);
			_displayFrames = 0;
			_displayTicks = 0;
			_displayFps = _displayFrameMs = 0.0;
			ResetWindow();
			log_debug("[DBG] [PROF] Frame profiler %s", _mode == 0 ? "off" : (_mode == 1 ? "on (normal)" : "on (precise)"));
		}
		if (_mode == 0)
			return;
		if (device != _device && !CreateQueries(device))
		{
			log_debug("[DBG] [PROF] Could not create the queries");
			requestedMode = 0;
			return;
		}
		_context = context;

		// Close this frame's query set
		QuerySet& s = _sets[_cur];
		if (s.open)
		{
			AddTimestamp(context);
			context->End(s.disjoint);
			s.open = false;
			s.pending = true;
		}

		// The frame's CPU time. Long gaps (loading, menus without 3D) aren't frames.
		if (_lastBoundary != 0)
		{
			const LONGLONG ticks = now - _lastBoundary;
			if (TicksToMs((double)ticks) < 1000.0)
			{
				_frames++;
				_frameTicksSum += ticks;
				if (ticks < _frameTicksMin) _frameTicksMin = ticks;
				if (ticks > _frameTicksMax) _frameTicksMax = ticks;
				_topLevelSum += _topLevelTicks;
				// The on-screen frame rate, twice a second
				_displayFrames++;
				_displayTicks += ticks;
				if (TicksToMs((double)_displayTicks) >= 500.0)
				{
					_displayFrameMs = TicksToMs((double)_displayTicks) / _displayFrames;
					_displayFps = 1000.0 / _displayFrameMs;
					_displayFrames = 0;
					_displayTicks = 0;
				}
			}
		}
		_lastBoundary = now;
		_topLevelTicks = 0;
		// Events that were never closed (unbalanced Begin/End) would grow the stack forever
		if (_stack.size() > 8 || _overflow)
		{
			_stack.clear();
			_overflow = false;
		}

		Collect(context);
		if (_frames > 0 && GetTickCount64() - _windowStart >= REPORT_MS)
		{
			Report();
			ResetWindow();
		}

		// Open the next frame's query set, unless the GPU is more than NUM_QUERY_SETS frames
		// behind: then this frame has no GPU times. The events still open (the ones around
		// Present) keep their slots in the old set, so they get no GPU time.
		_cur = (_cur + 1) % NUM_QUERY_SETS;
		QuerySet& n = _sets[_cur];
		if (!n.pending)
		{
			n.numTimestamps = 0;
			n.events.clear();
			n.window = _window;
			context->Begin(n.disjoint);
			n.open = true;
			AddTimestamp(context);
		}
	}

	void Report()
	{
		const double frames = (double)_frames;
		const double frameMs = TicksToMs((double)_frameTicksSum) / frames;
		const double topMs = TicksToMs((double)_topLevelSum) / frames;
		double presentMs = 0.0;
		for (const Stats& st : _stats)
			if (st.name == L"Present")
				presentMs = TicksToMs((double)st.cpuIncl) / frames;
		const double gpuSelfMs = _gpuFrames > 0 ? _gpuSelfMsSum / _gpuFrames : 0.0;
		log_debug("[DBG] [PROF] ===== %s mode. %d frames, %0.1f fps. CPU frame: avg %0.3f ms, min %0.3f, max %0.3f",
			_mode == 2 ? "PRECISE" : "Normal", _frames, 1000.0 / frameMs, frameMs,
			TicksToMs((double)_frameTicksMin), TicksToMs((double)_frameTicksMax));
		log_debug("[DBG] [PROF] CPU: in events %0.3f ms (Present %0.3f), outside events (game, unannotated code) %0.3f ms. GPU frame span: %0.3f ms",
			topMs, presentMs, frameMs - topMs, _gpuFrames > 0 ? _gpuFrameMsSum / _gpuFrames : 0.0);
		log_debug("[DBG] [PROF] GPU work in events (sum of the GPU self times): %0.3f ms -> %0.1f fps if the GPU were the only limit%s",
			gpuSelfMs, gpuSelfMs > 0.0 ? 1000.0 / gpuSelfMs : 0.0,
			_mode == 2 ? "" : " (only reliable in precise mode)");
		if (_mode == 2)
			log_debug("[DBG] [PROF] Precise mode: the CPU waits for the GPU at every event, so the CPU times and the frame rate aren't representative");
		{
			// TEMPORARY (Ctrl+Alt+T): the RT shadow mask diagnostic this report measured
			extern int g_iRTDiagMode;
			static int s_lastRTDiagMode = g_iRTDiagMode;
			log_debug("[DBG] [PROF] RT shadow mask: %s%s",
				g_iRTDiagMode == 0 ? "normal" : (g_iRTDiagMode == 1 ? "no traversal (diagnostic)" :
					"heat map (diagnostic: the readback stalls the GPU, don't use these times)"),
				s_lastRTDiagMode != g_iRTDiagMode ? " (switched during this report: it mixes modes, discard it)" : "");
			s_lastRTDiagMode = g_iRTDiagMode;
		}
		log_debug("[DBG] [PROF] %-40s %8s %10s %10s %10s %10s", "event", "calls/fr", "CPU incl", "CPU self", "GPU incl", "GPU self");
		for (const Stats& st : _stats)
		{
			if (st.calls == 0)
				continue;
			char name[64];
			const int indent = st.minDepth < 10 ? st.minDepth * 2 : 20;
			snprintf(name, sizeof(name), "%*s%ls", indent, "", st.name.c_str());
			char gpuIncl[16], gpuSelf[16];
			if (st.gpuCalls > 0)
			{
				snprintf(gpuIncl, sizeof(gpuIncl), "%0.3f", st.gpuIncl / frames);
				snprintf(gpuSelf, sizeof(gpuSelf), "%0.3f", st.gpuSelf / frames);
			}
			else
			{
				snprintf(gpuIncl, sizeof(gpuIncl), "-");
				snprintf(gpuSelf, sizeof(gpuSelf), "-");
			}
			log_debug("[DBG] [PROF] %-40s %8.1f %10.3f %10.3f %10s %10s", name, st.calls / frames,
				TicksToMs((double)st.cpuIncl) / frames, TicksToMs((double)st.cpuSelf) / frames, gpuIncl, gpuSelf);
		}
	}
};

extern FrameProfiler g_FrameProfiler;

// Times the rest of the enclosing scope as an event of the frame profiler
class FrameProfilerScope
{
public:
	explicit FrameProfilerScope(LPCWSTR name)
	{
		_active = g_FrameProfiler.IsEnabled() && g_FrameProfiler.Context() != nullptr;
		if (_active)
			g_FrameProfiler.BeginEvent(g_FrameProfiler.Device(), g_FrameProfiler.Context(), name);
	}
	~FrameProfilerScope()
	{
		if (_active)
			g_FrameProfiler.EndEvent(g_FrameProfiler.Context());
	}
private:
	bool _active;
};

#define FRAME_PROFILE_CAT2(a, b) a##b
#define FRAME_PROFILE_CAT(a, b) FRAME_PROFILE_CAT2(a, b)
#define FRAME_PROFILE(name) FrameProfilerScope FRAME_PROFILE_CAT(_frameProfilerScope, __LINE__)(name)
