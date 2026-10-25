#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path
import pandas as pd

# usage: graph.py [file.csv]   (default d34-2026.csv, writes <file>.html)
CSV_PATH = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("d34-2026.csv")
OUT_HTML = CSV_PATH.with_suffix(".html")

SERIES = [
    "t_ambient",
    "t_indoor",
    "humidity",
    "outhumidity",
    "co2",
    "outpressure",
    "outrain",
    "pm01",
    "pm25",
    "pm40",
    "pm10",
]

def read_csv_strip_banner(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise FileNotFoundError(path)

    # Read raw, strip optional banner line if present
    raw = path.read_text(encoding="utf-8", errors="replace").splitlines()
    if raw and raw[0].startswith("Read SD card file ["):
        raw = raw[1:]

    # Parse with pandas from in-memory text
    from io import StringIO
    df = pd.read_csv(StringIO("\n".join(raw)))

    if "timedate" not in df.columns:
        raise ValueError("Missing 'timedate' column.")

    df["timedate"] = pd.to_datetime(df["timedate"], format="%d-%m-%Y %H:%M", errors="raise")
    df = df.sort_values("timedate")
    return df

JS = r"""
// Week / month view with scrolling. TIMES = every sample as naive time in ms (UTC getters give the clock time).
(function () {
  var gd = document.getElementById('{plot_id}');
  var TIMES = __TIMES__;
  var DAY = 86400000;
  var tmin = TIMES[0], tmax = TIMES[TIMES.length - 1];
  var mode = 'week';      // default view: the most recent week
  var start;              // start of the visible window (ms)

  function pad(n) { return (n < 10 ? '0' : '') + n; }
  function fmt(ms) {
    var d = new Date(ms);
    return d.getUTCFullYear() + '-' + pad(d.getUTCMonth() + 1) + '-' + pad(d.getUTCDate()) + ' ' +
           pad(d.getUTCHours()) + ':' + pad(d.getUTCMinutes());
  }
  function weekStart(ms) {                         // Monday 00:00
    var d = new Date(ms); d.setUTCHours(0, 0, 0, 0);
    var dow = (d.getUTCDay() + 6) % 7;
    return d.getTime() - dow * DAY;
  }
  function monthStart(ms) { var d = new Date(ms); return Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), 1); }
  function nextMonth(ms) { var d = new Date(ms); return Date.UTC(d.getUTCFullYear(), d.getUTCMonth() + 1, 1); }
  function prevMonth(ms) { var d = new Date(ms); return Date.UTC(d.getUTCFullYear(), d.getUTCMonth() - 1, 1); }
  function weekOfYear(ms) {                        // ISO week number
    var d = new Date(ms); d.setUTCHours(0, 0, 0, 0);
    d.setUTCDate(d.getUTCDate() + 3 - (d.getUTCDay() + 6) % 7);
    var w1 = new Date(Date.UTC(d.getUTCFullYear(), 0, 4));
    return 1 + Math.round(((d - w1) / DAY - 3 + (w1.getUTCDay() + 6) % 7) / 7);
  }
  var MONTHS = ['januari','februari','maart','april','mei','juni','juli','augustus','september','oktober','november','december'];

  function end() { return mode === 'week' ? start + 7 * DAY : nextMonth(start); }
  function clamp() {
    var first = mode === 'week' ? weekStart(tmin) : monthStart(tmin);
    var last = mode === 'week' ? weekStart(tmax) : monthStart(tmax);
    if (start < first) start = first;
    if (start > last) start = last;
  }

  // y axis fitted on the visible samples of the visible (not legend-hidden) series
  function fitY(a, b) {
    var lo = Infinity, hi = -Infinity;
    gd.data.forEach(function (tr) {
      if (tr.visible === 'legendonly' || tr.visible === false) return;
      var y = tr.y;
      for (var i = 0; i < TIMES.length; i++) {
        if (TIMES[i] < a || TIMES[i] >= b) continue;
        var v = y[i];
        if (v === null || v === undefined || !isFinite(v)) continue;
        if (v < lo) lo = v;
        if (v > hi) hi = v;
      }
    });
    if (!isFinite(lo)) return null;
    var pad_ = (hi - lo) * 0.05 || 1;
    return [lo - pad_, hi + pad_];
  }

  var label;
  function show() {
    clamp();
    var a = start, b = end();
    var upd = { 'xaxis.range': [fmt(a), fmt(b)] };
    var yr = fitY(a, b);
    if (yr) upd['yaxis.range'] = yr;
    Plotly.relayout(gd, upd);
    var d = new Date(a);
    label.textContent = mode === 'week'
      ? 'Week ' + weekOfYear(a) + ' ' + d.getUTCFullYear() + '  (' + fmt(a).slice(0, 10) + ' - ' + fmt(b - DAY).slice(0, 10) + ')'
      : MONTHS[d.getUTCMonth()] + ' ' + d.getUTCFullYear();
    btnW.className = mode === 'week' ? 'on' : '';
    btnM.className = mode === 'month' ? 'on' : '';
  }
  function setMode(m) {                            // keep the window around the current start
    var cur = start;
    mode = m;
    start = m === 'week' ? weekStart(cur) : monthStart(cur);
    show();
  }
  function move(dir) {
    if (mode === 'week') start += dir * 7 * DAY;
    else start = dir > 0 ? nextMonth(start) : prevMonth(start);
    show();
  }
  function latest() { start = mode === 'week' ? weekStart(tmax) : monthStart(tmax); show(); }

  // toolbar
  var css = document.createElement('style');
  css.textContent = '#gtb{font-family:sans-serif;margin:8px 12px;display:flex;gap:6px;align-items:center}' +
    '#gtb button{padding:5px 12px;border:1px solid #888;background:#f4f4f4;border-radius:4px;cursor:pointer;font-size:14px}' +
    '#gtb button.on{background:#2a6fdb;color:#fff;border-color:#2a6fdb}#gtb span{margin-left:10px;font-size:15px;font-weight:bold}';
  document.head.appendChild(css);
  var bar = document.createElement('div'); bar.id = 'gtb';
  function mk(text, fn) { var b = document.createElement('button'); b.textContent = text; b.onclick = fn; bar.appendChild(b); return b; }
  var btnW = mk('Week', function () { setMode('week'); });
  var btnM = mk('Maand', function () { setMode('month'); });
  mk('◀', function () { move(-1); });
  mk('▶', function () { move(1); });
  mk('Laatste', latest);
  label = document.createElement('span'); bar.appendChild(label);
  gd.parentNode.insertBefore(bar, gd);

  document.addEventListener('keydown', function (e) {
    if (e.key === 'ArrowLeft') move(-1);
    else if (e.key === 'ArrowRight') move(1);
  });
  // legend click: fit y again
  gd.on('plotly_restyle', function () { var yr = fitY(start, end()); if (yr) Plotly.relayout(gd, { 'yaxis.range': yr }); });

  latest();
})();
"""


def main() -> int:
    try:
        df = read_csv_strip_banner(CSV_PATH)
    except Exception as e:
        print(f"ERROR reading CSV: {e}")
        return 1

    # Keep only existing series
    cols = [c for c in SERIES if c in df.columns]
    if not cols:
        print("ERROR: None of the requested series columns exist.")
        print("Available columns:", list(df.columns))
        return 1

    # Convert to numeric
    for c in cols:
        df[c] = pd.to_numeric(df[c], errors="coerce")

    import plotly.graph_objects as go

    fig = go.Figure()

    for c in cols:
        fig.add_trace(
            go.Scatter(
                x=df["timedate"],
                y=df[c],
                mode="lines+markers",
                name=c,
                connectgaps=True,
            )
        )

    fig.update_layout(
        title=str(CSV_PATH),
        dragmode="pan",
        xaxis_title="Time",
        yaxis_title="Value",
        hovermode="x unified",
        legend_title="Click to toggle (double-click to isolate)",
        template="plotly_white",
    )

    # sample times as naive ms (the clock time of the CSV), used by the week / month scrolling
    times = (df["timedate"].astype("datetime64[ns]").astype("int64") // 1_000_000).tolist()
    script = JS.replace("__TIMES__", json.dumps(times))
    fig.write_html(OUT_HTML, include_plotlyjs="cdn", auto_open=True, post_script=script)
    print(f"OK: wrote {OUT_HTML.resolve()} (opened in browser).")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
