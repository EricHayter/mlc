"use strict";

// Bundled sample so "Load sample" works offline / from file:// without fetch.
const SAMPLE_CSV = `buffer size (kb), average latency per load (ns), cycles per load
8, 1.66, 4.13
10, 1.44, 4.09
12, 1.42, 4.08
15, 1.42, 4.09
18, 1.59, 4.37
22, 1.57, 4.51
27, 1.47, 4.21
33, 2.32, 6.59
41, 4.31, 12.30
51, 4.28, 12.26
63, 4.26, 12.25
78, 4.40, 12.62
97, 4.32, 12.32
121, 4.31, 12.36
151, 4.47, 12.81
188, 4.39, 12.57
235, 4.61, 13.13
293, 7.04, 20.17
366, 9.15, 26.31
`;

const els = {
  drop: document.getElementById("drop"),
  file: document.getElementById("file"),
  sample: document.getElementById("sample"),
  pastebox: document.getElementById("pastebox"),
  pasteload: document.getElementById("pasteload"),
  status: document.getElementById("status"),
  chartwrap: document.getElementById("chartwrap"),
  meta: document.getElementById("meta"),
  canvas: document.getElementById("chart"),
};

let chart = null;

function setStatus(msg, isError) {
  els.status.textContent = msg || "";
  els.status.classList.toggle("error", !!isError);
}

// Parse the fixed 3-column mlc CSV. Tolerates the header row, blank lines,
// and surrounding whitespace. Returns { rows: [{size, ns, cycles}], skipped }.
function parseCsv(text) {
  const rows = [];
  let skipped = 0;
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (line === "") continue;
    const parts = line.split(",").map((s) => s.trim());
    if (parts.length < 3) { skipped++; continue; }
    const size = Number(parts[0]);
    const ns = Number(parts[1]);
    const cycles = Number(parts[2]);
    if (!Number.isFinite(size) || !Number.isFinite(ns) || !Number.isFinite(cycles)) {
      skipped++; // header row lands here and is silently dropped
      continue;
    }
    rows.push({ size, ns, cycles });
  }
  rows.sort((a, b) => a.size - b.size);
  return { rows, skipped };
}

function render(text, sourceName) {
  const { rows } = parseCsv(text);
  if (rows.length === 0) {
    setStatus("No numeric data rows found in that input.", true);
    return;
  }

  const points = rows.map((r) => ({ x: r.size, ns: r.ns, cycles: r.cycles }));

  const data = {
    datasets: [
      {
        label: "latency (ns)",
        yAxisID: "yns",
        data: points.map((p) => ({ x: p.x, y: p.ns })),
        borderColor: "#5b9dff",
        backgroundColor: "#5b9dff",
        tension: 0.15,
        pointRadius: 3,
      },
      {
        label: "cycles",
        yAxisID: "ycyc",
        data: points.map((p) => ({ x: p.x, y: p.cycles })),
        borderColor: "#ffb454",
        backgroundColor: "#ffb454",
        tension: 0.15,
        pointRadius: 3,
      },
    ],
  };

  const gridColor = "#262b38";
  const tickColor = "#8b93a7";
  const options = {
    responsive: true,
    maintainAspectRatio: false,
    interaction: { mode: "index", intersect: false },
    scales: {
      x: {
        type: "logarithmic",
        title: { display: true, text: "buffer size (KiB)", color: tickColor },
        grid: { color: gridColor },
        ticks: { color: tickColor },
      },
      yns: {
        type: "linear",
        position: "left",
        beginAtZero: true,
        title: { display: true, text: "ns / load", color: "#5b9dff" },
        grid: { color: gridColor },
        ticks: { color: "#5b9dff" },
      },
      ycyc: {
        type: "linear",
        position: "right",
        beginAtZero: true,
        title: { display: true, text: "cycles / load", color: "#ffb454" },
        grid: { drawOnChartArea: false },
        ticks: { color: "#ffb454" },
      },
    },
    plugins: {
      legend: { labels: { color: "#222" } },
      tooltip: {
        callbacks: {
          title: (items) => `${items[0].parsed.x} KiB`,
        },
      },
    },
  };

  // Unhide the container *before* constructing the chart: Chart.js measures the
  // canvas's container at creation time, and a hidden (display:none) container
  // reports 0x0, so the chart renders blank until a later resize. This is what
  // made piped reports show an empty graph until Firefox was reopened.
  els.chartwrap.hidden = false;

  if (chart) chart.destroy();
  chart = new Chart(els.canvas.getContext("2d"), { type: "line", data, options });

  els.meta.textContent =
    `${sourceName} — ${rows.length} points, ` +
    `${rows[0].size}–${rows[rows.length - 1].size} KiB`;
  setStatus(`Loaded ${rows.length} points.`);
}

function readFile(file) {
  const reader = new FileReader();
  reader.onload = () => render(String(reader.result), file.name);
  reader.onerror = () => setStatus(`Could not read ${file.name}.`, true);
  reader.readAsText(file);
}

// --- Wiring -----------------------------------------------------------------

els.file.addEventListener("change", () => {
  if (els.file.files.length) readFile(els.file.files[0]);
});

els.sample.addEventListener("click", () => render(SAMPLE_CSV, "sample"));

els.pasteload.addEventListener("click", () => {
  const text = els.pastebox.value;
  if (text.trim() === "") { setStatus("Paste box is empty.", true); return; }
  render(text, "pasted data");
});

["dragenter", "dragover"].forEach((ev) =>
  els.drop.addEventListener(ev, (e) => {
    e.preventDefault();
    els.drop.classList.add("over");
  })
);
["dragleave", "drop"].forEach((ev) =>
  els.drop.addEventListener(ev, (e) => {
    e.preventDefault();
    els.drop.classList.remove("over");
  })
);
els.drop.addEventListener("drop", (e) => {
  const file = e.dataTransfer.files[0];
  if (file) readFile(file);
});

// If mlc-view piped data in, plot it immediately.
(function autoload() {
  const inline = document.getElementById("mlc-data");
  const text = inline ? inline.textContent.trim() : "";
  if (text !== "") render(text, "piped from mlc");
})();
