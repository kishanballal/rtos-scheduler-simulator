// sim.js — a line-by-line JavaScript port of the C++ engine (src/Simulator.cpp,
// src/Schedulers.cpp, src/Statistics.cpp, src/Schedulability.cpp) so the web page
// produces the same timelines and numbers as rtos_sim. Comments name the C++
// source each piece mirrors. No dependencies; works in a browser and in Node.
(function (root) {
  "use strict";

  const NO_DEADLINE = Infinity;   // kNoDeadline
  const IDLE = -1;                // kIdle

  // ---- Task (Task.cpp) ------------------------------------------------------
  function makeTask(t) {
    const task = {
      id: t.id | 0, name: String(t.name || "").trim(), arrival: +t.arrival, burst: +t.burst,
      priority: +t.priority, period: +t.period || 0, deadline: +t.deadline || 0, cs: t.cs || null,
    };
    for (const k of ["id", "arrival", "burst", "priority", "period", "deadline"])
      if (!Number.isInteger(task[k])) throw new Error(`task ${task.name || "?"}: ${k} must be a whole number`);
    if (task.cs) {
      if (!task.cs.resource) throw new Error("resource name must not be empty");
      if (task.cs.start < 0 || task.cs.length <= 0 || task.cs.start + task.cs.length > task.burst)
        throw new Error(`task ${task.name}: critical section must lie inside the burst`);
    }
    if (task.id < 0) throw new Error("task id must be >= 0");
    if (!task.name) throw new Error("task name must not be empty");
    if (task.arrival < 0) throw new Error(`task ${task.name}: arrival must be >= 0`);
    if (task.burst <= 0) throw new Error(`task ${task.name}: burst must be > 0`);
    if (task.period < 0) throw new Error(`task ${task.name}: period must be >= 0`);
    if (task.deadline < 0) throw new Error(`task ${task.name}: deadline must be >= 0`);
    task.periodic = task.period > 0;
    task.relDeadline = task.deadline > 0 ? task.deadline : task.period > 0 ? task.period : NO_DEADLINE;
    task.utilization = task.periodic ? task.burst / task.period : 0;
    return task;
  }

  // ---- CSV loader (TaskLoader.cpp) -----------------------------------------
  function parseCsv(text) {
    const tasks = [];
    let seenFirst = false;
    text.split(/\r?\n/).forEach((raw, i) => {
      const line = raw.trim();
      if (!line || line[0] === "#") return;
      if (!seenFirst) { seenFirst = true; if (!/^\d/.test(line)) return; }   // header row
      const f = line.split(",").map((s) => s.trim());
      if (f.length !== 7 && f.length !== 8) throw new Error(`line ${i + 1}: expected 7 or 8 fields, got ${f.length}`);
      const num = (s) => { if (!/^-?\d+$/.test(s)) throw new Error(`line ${i + 1}: not a whole number: '${s}'`); return +s; };
      let cs = null;
      if (f.length === 8 && f[7]) {
        const p = f[7].split(":").map((s) => s.trim());
        if (p.length !== 3) throw new Error(`line ${i + 1}: critical section must be RESOURCE:START:LENGTH`);
        cs = { resource: p[0], start: num(p[1]), length: num(p[2]) };
      }
      tasks.push({ id: num(f[0]), name: f[1], arrival: num(f[2]), burst: num(f[3]), priority: num(f[4]),
                   period: num(f[5]), deadline: num(f[6]), cs });
    });
    return tasks;
  }

  function toCsv(tasks) {
    const rows = ["id,name,arrival,burst,priority,period,deadline,resource"];
    for (const t of tasks)
      rows.push([t.id, t.name, t.arrival, t.burst, t.priority, t.period, t.deadline,
                 t.cs ? `${t.cs.resource}:${t.cs.start}:${t.cs.length}` : ""].join(","));
    return rows.join("\n") + "\n";
  }

  // ---- Schedulers (Schedulers.hpp / .cpp) ----------------------------------
  // Tuple comparison (key, release, task id): a strict total order on jobs, so a
  // sorted array picks exactly what the C++ MinHeap picks.
  const tupleLess = (key) => (a, b) => {
    const ka = key(a), kb = key(b);
    if (ka !== kb) return ka < kb;
    if (a.release !== b.release) return a.release < b.release;
    return a.task.id < b.task.id;
  };
  const byPriority = tupleLess((j) => j.effectivePriority);
  const byPeriod = tupleLess((j) => (j.task.periodic ? j.task.period : NO_DEADLINE));
  const byDeadline = tupleLess((j) => j.absDeadline);

  class FcfsScheduler {
    constructor() { this.ready = []; }
    name() { return "FCFS"; }
    onJobArrival(j) { this.ready.push(j); }
    pickNext(running) {
      if (running) return running;
      return this.ready.length ? this.ready.shift() : null;
    }
    onJobComplete() {}
    onPriorityChanged() {}
    hasReadyJobs() { return this.ready.length > 0; }
  }

  class RoundRobinScheduler {
    constructor(q) {
      if (!(q > 0)) throw new Error("Round Robin quantum must be > 0");
      this.q = q; this.used = 0; this.ready = [];
    }
    name() { return `RR(q=${this.q})`; }
    onJobArrival(j) { this.ready.push(j); }
    pickNext(running) {
      if (running && this.used < this.q) { ++this.used; return running; }
      if (running) {
        if (!this.ready.length) { this.used = 1; return running; }
        this.ready.push(running);
      }
      if (!this.ready.length) { this.used = 0; return null; }
      this.used = 1;
      return this.ready.shift();
    }
    onJobComplete() { this.used = 0; }
    onPriorityChanged() {}
    hasReadyJobs() { return this.ready.length > 0; }
  }

  class HeapScheduler {
    constructor(label, less) { this.label = label; this.less = less; this.ready = []; }
    name() { return this.label; }
    top() {
      let best = 0;
      for (let i = 1; i < this.ready.length; ++i) if (this.less(this.ready[i], this.ready[best])) best = i;
      return best;
    }
    pop() { return this.ready.splice(this.top(), 1)[0]; }
    onJobArrival(j) { this.ready.push(j); }
    pickNext(running) {
      if (!this.ready.length) return running;
      if (!running) return this.pop();
      if (this.less(this.ready[this.top()], running)) {   // strictly more urgent: preempt
        const next = this.pop();
        this.ready.push(running);
        return next;
      }
      return running;
    }
    onJobComplete() {}
    onPriorityChanged() {}   // ordering is recomputed on every top()
    hasReadyJobs() { return this.ready.length > 0; }
  }

  const SCHEDULER_KEYS = ["fcfs", "rr", "priority", "rm", "edf"];
  function makeScheduler(key, quantum) {
    switch (key) {
      case "fcfs": return new FcfsScheduler();
      case "rr": return new RoundRobinScheduler(quantum);
      case "priority": return new HeapScheduler("Priority", byPriority);
      case "rm": return new HeapScheduler("RM", byPeriod);
      case "edf": return new HeapScheduler("EDF", byDeadline);
    }
    throw new Error(`unknown scheduler '${key}'`);
  }

  // ---- std::mt19937 + libstdc++ uniform_int_distribution -------------------
  // Matches GCC's output so --jitter/--seed timelines equal the CLI's.
  function mt19937(seed) {
    const mt = new Uint32Array(624);
    let idx = 624;
    mt[0] = seed >>> 0;
    for (let i = 1; i < 624; ++i) {
      const p = mt[i - 1] ^ (mt[i - 1] >>> 30);
      mt[i] = (Math.imul(p, 1812433253) + i) >>> 0;
    }
    return function next() {
      if (idx >= 624) {
        for (let i = 0; i < 624; ++i) {
          const y = (mt[i] & 0x80000000) | (mt[(i + 1) % 624] & 0x7fffffff);
          mt[i] = mt[(i + 397) % 624] ^ (y >>> 1) ^ (y & 1 ? 0x9908b0df : 0);
        }
        idx = 0;
      }
      let y = mt[idx++];
      y ^= y >>> 11;
      y ^= (y << 7) & 0x9d2c5680;
      y ^= (y << 15) & 0xefc60000;
      y ^= y >>> 18;
      return y >>> 0;
    };
  }
  function uniformInt(gen, max) {   // [0, max], Lemire's method as in libstdc++
    const range = BigInt(max) + 1n;
    let product = BigInt(gen()) * range;
    let low = product & 0xffffffffn;
    if (low < range) {
      const threshold = ((1n << 32n) - range) % range;
      while (low < threshold) { product = BigInt(gen()) * range; low = product & 0xffffffffn; }
    }
    return Number(product >> 32n);
  }

  // ---- Simulator (Simulator.cpp) -------------------------------------------
  const gcd = (a, b) => (b ? gcd(b, a % b) : a);
  const lcm = (a, b) => (a / gcd(a, b)) * b;

  function prepare(rawTasks) {
    const tasks = rawTasks.map(makeTask);
    const seen = new Set();
    for (const t of tasks) {
      if (seen.has(t.id)) throw new Error(`duplicate task id ${t.id}`);
      seen.add(t.id);
    }
    // TaskLoader stable-sorts by arrival; Array.prototype.sort is stable too.
    return tasks.sort((a, b) => a.arrival - b.arrival);
  }

  function defaultHorizon(tasks) {
    const CAP = 10000;
    let hyper = 0, maxArrival = 0, totalBurst = 0;
    for (const t of tasks) {
      maxArrival = Math.max(maxArrival, t.arrival);
      totalBurst += t.burst;
      if (t.periodic) hyper = hyper === 0 ? t.period : lcm(hyper, t.period);
      if (hyper > CAP) return CAP;
    }
    const h = hyper > 0 ? maxArrival + hyper : maxArrival + totalBurst;
    return Math.min(Math.max(h, 1), CAP);
  }

  function run(tasks, opts) {
    const { algo, quantum = 2, inheritance = false, jitter = 0, seed = 1 } = opts;
    const horizon = opts.horizon > 0 ? opts.horizon : defaultHorizon(tasks);
    const sched = makeScheduler(algo, quantum);
    const result = { schedulerName: sched.name() + (inheritance ? "+PI" : ""), timeline: [], jobs: [],
                     contextSwitches: 0, blockings: 0, horizon, events: [] };

    // Event queue of future releases, ordered by (time, task id).
    const events = tasks.map((t) => ({ time: t.arrival, task: t }));
    const popEvent = () => {
      let b = 0;
      for (let i = 1; i < events.length; ++i) {
        const e = events[i], f = events[b];
        if (e.time < f.time || (e.time === f.time && e.task.id < f.task.id)) b = i;
      }
      return b;
    };
    const rng = mt19937(seed);

    // MutexTable
    const mutexes = new Map();
    const mutex = (name) => { if (!mutexes.has(name)) mutexes.set(name, { owner: null, waiters: [] }); return mutexes.get(name); };
    const progress = (j) => j.task.burst - j.remaining;
    const acquireOrBlock = (job, now) => {
      const cs = job.task.cs;
      if (!cs || progress(job) !== cs.start) return true;
      const m = mutex(cs.resource);
      if (!m.owner || m.owner === job) {
        if (!m.owner) result.events.push({ t: now, text: `${job.task.name} locks ${cs.resource}` });
        m.owner = job;
        return true;
      }
      m.waiters.push(job);
      ++result.blockings;
      result.events.push({ t: now, text: `${job.task.name} blocks on ${cs.resource} (held by ${m.owner.task.name})` });
      if (inheritance && job.effectivePriority < m.owner.effectivePriority) {
        m.owner.effectivePriority = job.effectivePriority;
        result.events.push({ t: now, text: `${m.owner.task.name} inherits priority ${job.effectivePriority}` });
        sched.onPriorityChanged(m.owner);
      }
      return false;
    };
    const releaseIfDone = (job, now) => {
      const cs = job.task.cs;
      if (!cs) return;
      const m = mutex(cs.resource);
      if (m.owner !== job) return;
      if (progress(job) < cs.start + cs.length && job.remaining > 0) return;
      m.owner = null;
      result.events.push({ t: now + 1, text: `${job.task.name} unlocks ${cs.resource}` });
      if (job.effectivePriority !== job.task.priority) {
        job.effectivePriority = job.task.priority;
        sched.onPriorityChanged(job);
      }
      if (!m.waiters.length) return;
      let best = 0;
      for (let i = 1; i < m.waiters.length; ++i)
        if (m.waiters[i].effectivePriority < m.waiters[best].effectivePriority) best = i;
      sched.onJobArrival(m.waiters.splice(best, 1)[0]);
    };

    let running = null, lastRun = null;
    for (let now = 0; now < horizon; ++now) {
      // 1. Releases due now.
      while (events.length && events[popEvent()].time <= now) {
        const ev = events.splice(popEvent(), 1)[0];
        const t = ev.task;
        const job = { jobId: result.jobs.length, task: t, release: ev.time, remaining: t.burst,
                      absDeadline: t.relDeadline === NO_DEADLINE ? NO_DEADLINE : ev.time + t.relDeadline,
                      start: null, finish: null, effectivePriority: t.priority };
        result.jobs.push(job);
        sched.onJobArrival(job);
        if (t.periodic) events.push({ time: ev.time + t.period + (jitter > 0 ? uniformInt(rng, jitter) : 0), task: t });
      }
      // 2. Done?
      if (!running && !sched.hasReadyJobs() && !events.length) break;
      // 3. Pick, retrying if the pick blocks on a mutex.
      let next = sched.pickNext(running, now);
      for (;;) {
        if (next !== running) running = null;
        if (!next || acquireOrBlock(next, now)) break;
        running = null;
        next = sched.pickNext(null, now);
      }
      if (!next) { result.timeline.push(IDLE); continue; }
      if (lastRun && next !== lastRun) ++result.contextSwitches;
      // 4. One tick of work.
      if (next.start === null) next.start = now;
      --next.remaining;
      result.timeline.push(next.task.id);
      lastRun = next;
      releaseIfDone(next, now);
      // 5. Completion.
      if (next.remaining === 0) { next.finish = now + 1; sched.onJobComplete(next); running = null; }
      else running = next;
    }
    result.length = result.timeline.length;
    result.stats = computeStats(result, tasks);
    return result;
  }

  // ---- Statistics (Statistics.cpp) -----------------------------------------
  function computeStats(result, tasks) {
    const rows = new Map();
    const perTask = tasks.map((t) => {
      const s = { taskId: t.id, name: t.name, released: 0, completed: 0, deadlineMisses: 0,
                  sumW: 0, sumT: 0, sumR: 0, misses: [] };
      rows.set(t.id, s);
      return s;
    });
    const o = { length: result.length, contextSwitches: result.contextSwitches, completed: 0,
                deadlineMisses: 0, sumW: 0, sumT: 0, sumR: 0, cpuUtilization: 0, throughput: 0 };
    for (const j of result.jobs) {
      const s = rows.get(j.task.id);
      ++s.released;
      const missed = j.finish !== null ? j.finish > j.absDeadline : j.absDeadline <= result.length;
      if (missed) { ++s.deadlineMisses; ++o.deadlineMisses; s.misses.push(j.absDeadline); j.missed = true; }
      if (j.finish === null) continue;
      ++s.completed; ++o.completed;
      const tat = j.finish - j.release, w = tat - j.task.burst, r = j.start - j.release;
      s.sumW += w; s.sumT += tat; s.sumR += r;
      o.sumW += w; o.sumT += tat; o.sumR += r;
    }
    const avg = (sum, n) => (n > 0 ? sum / n : 0);
    for (const s of perTask) {
      s.avgWaiting = avg(s.sumW, s.completed);
      s.avgTurnaround = avg(s.sumT, s.completed);
      s.avgResponse = avg(s.sumR, s.completed);
    }
    const busy = result.timeline.filter((id) => id !== IDLE).length;
    if (result.length > 0) { o.cpuUtilization = busy / result.length; o.throughput = o.completed / result.length; }
    o.avgWaiting = avg(o.sumW, o.completed);
    o.avgTurnaround = avg(o.sumT, o.completed);
    o.avgResponse = avg(o.sumR, o.completed);
    return { perTask, overall: o };
  }

  // ---- Schedulability (Schedulability.cpp) ---------------------------------
  function schedulability(tasks) {
    const EPS = 1e-9;
    let n = 0, implicit = true, u = 0;
    for (const t of tasks) {
      u += t.utilization;
      if (!t.periodic) continue;
      ++n;
      if (t.relDeadline !== t.period) implicit = false;
    }
    const bound = n === 0 ? 1 : n * (Math.pow(2, 1 / n) - 1);
    return { periodicTasks: n, utilization: u, rmBound: bound, implicitDeadlines: implicit,
             rmGuaranteed: u <= bound + EPS, edfFeasible: u <= 1 + EPS };
  }

  const api = { NO_DEADLINE, IDLE, SCHEDULER_KEYS, parseCsv, toCsv, prepare, defaultHorizon, run, schedulability };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.RtosSim = api;
})(typeof window !== "undefined" ? window : globalThis);
