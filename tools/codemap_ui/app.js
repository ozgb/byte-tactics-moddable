"use strict";

const SUB_COLORS = {
  presentation: "#b38cf0", game: "#f0b45a", network: "#5ac8e8", harness: "#8a8f98", units: "#6fcf7f",
  weapons: "#ef6f6c", players: "#f28cc0", features: "#9ccc65", wind_meteors: "#4dd0c3", orders: "#e3c35a",
  ai: "#d08bf0", pathfinding: "#64b5f6", targeting: "#ff8a65", los_radar: "#4fc3f7", economy: "#ffd54f",
  map: "#b09184", files: "#90a4ae", platform: "#78909c", seams: "#b0bec5",
};
const BANDS = ["Drivers", "Tick phases", "Called from the phases", "World and engine"];
const QUIET_TARGETS = new Set(["platform", "files", "harness"]);
const GITHUB = "https://github.com/ozgb/byte-tactics-moddable";
const START_HERE = [
  ["RunGameSteps", "every phase of one simulation tick, in order"],
  ["MainLoopTick", "what happens each frame, before and after the tick"],
  ["UpdateAllUnits", "the per-unit update: orders, weapons, scripts"],
  ["UpdatePlayers", "economy, AI, pathfinding, line of sight per player"],
  ["HandleNetPackets", "how network input reaches the simulation"],
];
const ROW_CAP = 30;
const DOC_SECTIONS = new Set(["wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar", "orders"]);

const $ = (selector) => document.querySelector(selector);
const stage = $("#stage");
const side = $("#side");

const state = {
  index: null,
  fnKey: new Map(),
  shortNames: new Map(),
  itemNames: new Set(),
  typeNames: new Set(),
  fnCache: new Map(),
  srcCache: new Map(),
  searchRows: [],
  route: null,
  current: null,
  trail: [],
  sideTab: "source",
  expanded: { left: false, right: false },
  source: null,
  prefs: loadPrefs(),
};

function recall(key) {
  try {
    return localStorage.getItem(key);
  } catch {
    return null;
  }
}

function remember(key, value) {
  try {
    localStorage.setItem(key, value);
  } catch {
    return;
  }
}

function loadPrefs() {
  const defaults = { view: "flow", statements: false, harness: false, showHarnessOnMap: false };
  try {
    return { ...defaults, ...JSON.parse(recall("codemap.prefs") || "{}") };
  } catch {
    return defaults;
  }
}

const savePrefs = () => remember("codemap.prefs", JSON.stringify(state.prefs));

const esc = (value) =>
  String(value ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
const color = (sub) => SUB_COLORS[sub] || "#808890";
const subStyle = (sub) => `style="--c-sub:${color(sub)}"`;
const enc = encodeURIComponent;

const fnHref = (name, file) => `#/fn/${enc(`${name}@${file}`)}`;
const fnHrefById = (id) => {
  const row = state.index.functions[id];
  return fnHref(row[0], row[1]);
};
const itemHref = (name) => `#/item/${enc(name)}`;
const typeHref = (name) => `#/type/${enc(name)}`;
const fileHref = (path) => `#/file/${enc(path)}`;
const subHref = (name) => `#/sub/${enc(name)}`;
const edgeHref = (a, b) => `#/edge/${enc(a)}/${enc(b)}`;
const subChip = (sub) => `<a class="chip" ${subStyle(sub)} href="${subHref(sub)}"><span class="dot"></span>${esc(sub)}</a>`;

async function api(route, params = {}) {
  const query = new URLSearchParams(params).toString();
  const response = await fetch(`/api/${route}${query ? `?${query}` : ""}`);
  if (!response.ok) throw new Error(`${route}: ${response.status}`);
  return response.json();
}

async function loadFunction(id) {
  if (!state.fnCache.has(id)) state.fnCache.set(id, await api("fn", { id }));
  return state.fnCache.get(id);
}

async function loadSource(path) {
  if (!state.srcCache.has(path)) state.srcCache.set(path, (await api("src", { path })).text);
  return state.srcCache.get(path);
}

async function init() {
  const index = await api("index");
  state.index = index;
  index.functions.forEach((row, id) => {
    state.fnKey.set(`${row[0]}@${row[1]}`, id);
    const short = row[0].split("::").pop();
    if (!state.shortNames.has(short)) state.shortNames.set(short, []);
    state.shortNames.get(short).push(id);
  });
  index.items.forEach((row) => state.itemNames.add(row[0]));
  index.types.forEach((name) => state.typeNames.add(name));
  buildSearchRows();
  const tick = index.tick.function;
  if (tick !== null) $("#nav-tick").href = fnHrefById(tick);
  window.addEventListener("hashchange", route);
  route();
}

function buildSearchRows() {
  const { functions, items, types, files, subsystems } = state.index;
  const rows = [];
  subsystems.forEach((s) => rows.push({ kind: "system", text: s.name, href: subHref(s.name), where: `${s.functions} functions`, weight: 0 }));
  functions.forEach((row) => rows.push({ kind: "function", text: row[0], alt: row[3], href: fnHref(row[0], row[1]), where: row[1].replace(/^src\//, ""), sub: row[2], weight: 1 }));
  types.forEach((name) => rows.push({ kind: "type", text: name, href: typeHref(name), where: "", weight: 2 }));
  items.forEach((row) => rows.push({ kind: row[1] === "global" ? "global" : "field", text: row[0], href: itemHref(row[0]), where: row[2], weight: 3 }));
  files.forEach((path) => rows.push({ kind: "file", text: path, href: fileHref(path), where: "", weight: 4 }));
  rows.forEach((row) => {
    row.lower = row.text.toLowerCase();
    row.last = row.lower.split("::").pop();
  });
  state.searchRows = rows;
}

function searchMatches(query) {
  const q = query.trim().toLowerCase();
  if (!q) return [];
  const scored = [];
  const address = q.startsWith("0x") ? q : null;
  for (const row of state.searchRows) {
    let score = -1;
    if (address) {
      if (row.alt && row.alt.startsWith(address)) score = 0;
    } else if (row.last === q || row.lower === q) score = 0;
    else if (row.last.startsWith(q)) score = 1;
    else if (row.lower.startsWith(q)) score = 2;
    else if (row.lower.includes(q)) score = 3;
    if (score >= 0) scored.push([score, row.weight, row.text.length, row]);
  }
  scored.sort((a, b) => a[0] - b[0] || a[1] - b[1] || a[2] - b[2]);
  return scored.slice(0, 40).map((entry) => entry[3]);
}

function setupSearch() {
  const input = $("#search");
  const panel = $("#results");
  let selected = 0;
  let rows = [];
  const draw = () => {
    panel.hidden = rows.length === 0;
    panel.innerHTML = rows
      .map((row, i) => `<a class="result${i === selected ? " sel" : ""}" href="${row.href}" role="option">
        <span class="kind">${row.kind}</span><span class="name">${esc(row.text)}</span>
        <span class="where">${esc(row.where)}</span></a>`)
      .join("");
    panel.querySelector(".sel")?.scrollIntoView({ block: "nearest" });
  };
  const close = () => {
    rows = [];
    draw();
  };
  input.addEventListener("input", () => {
    rows = searchMatches(input.value);
    selected = 0;
    draw();
  });
  input.addEventListener("keydown", (event) => {
    if (event.key === "ArrowDown" || event.key === "ArrowUp") {
      event.preventDefault();
      selected = (selected + (event.key === "ArrowDown" ? 1 : -1) + rows.length) % Math.max(rows.length, 1);
      draw();
    } else if (event.key === "Enter" && rows[selected]) {
      location.hash = rows[selected].href;
      input.value = "";
      input.blur();
      close();
    } else if (event.key === "Escape") {
      input.blur();
      close();
    }
  });
  panel.addEventListener("click", () => {
    input.value = "";
    close();
  });
  document.addEventListener("click", (event) => {
    if (!event.target.closest("#search-wrap")) close();
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "/" && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
      event.preventDefault();
      input.focus();
      input.select();
    }
  });
}

function setupDivider() {
  const divider = $("#divider");
  state.sideWidth = recall("codemap.side");
  divider.addEventListener("pointerdown", (event) => {
    divider.setPointerCapture(event.pointerId);
    divider.classList.add("drag");
  });
  divider.addEventListener("pointermove", (event) => {
    if (divider.classList.contains("drag")) side.style.width = `${window.innerWidth - event.clientX}px`;
  });
  divider.addEventListener("pointerup", () => {
    divider.classList.remove("drag");
    state.sideWidth = side.style.width;
    remember("codemap.side", side.style.width);
    if (state.current?.redraw) state.current.redraw();
  });
}

async function route() {
  const parts = location.hash.replace(/^#\/?/, "").split("/").map(decodeURIComponent);
  const [view, a, b] = parts;
  document.querySelectorAll("#bar nav a").forEach((link) => link.classList.remove("on"));
  const token = (state.route = Symbol());
  state.expanded = { left: false, right: false };
  const onMap = !["fn", "item", "type", "file"].includes(view);
  side.style.width = state.sideWidth || (onMap ? "32%" : "44%");
  try {
    if (view === "fn") await showFunction(a, token);
    else if (view === "item") await showItem(a, token);
    else if (view === "type") await showType(a, token);
    else if (view === "file") await showFile(a, token);
    else await showMap(view === "sub" ? { sub: a } : view === "edge" ? { edge: [a, b] } : {}, token);
  } catch (error) {
    stage.innerHTML = `<p class="empty">Could not load this view: ${esc(error.message)}</p>`;
  }
  stage.scrollTop = 0;
}

function pushTrail(entry) {
  state.trail = [entry, ...state.trail.filter((e) => e.href !== entry.href)].slice(0, 6);
  $("#trail").innerHTML = state.trail
    .slice(1)
    .map((e) => `<a href="${e.href}" title="${esc(e.title)}">${esc(e.label)}</a>`)
    .join("");
}

const NODE_W = 182;
const NODE_H = 94;
const BAND_H = 150;
const MAP_W = 1240;

function mapLayout() {
  const nodes = new Map();
  BANDS.forEach((_, band) => {
    const members = state.index.subsystems.filter((s) => s.band === band && !s.hidden);
    const gap = 18;
    const total = members.length * NODE_W + (members.length - 1) * gap;
    let x = (MAP_W - total) / 2;
    const y = band * (BAND_H + 16) + 34;
    members.forEach((s) => {
      nodes.set(s.name, { ...s, x, y, cx: x + NODE_W / 2 });
      x += NODE_W + gap;
    });
  });
  return nodes;
}

function assignPorts(edges, nodes) {
  const ports = new Map();
  const sides = new Map();
  const add = (name, side, edge, end, otherX) => {
    const key = `${name}|${side}`;
    if (!sides.has(key)) sides.set(key, []);
    sides.get(key).push({ name, edge, end, otherX });
  };
  edges.forEach((edge) => {
    const a = nodes.get(edge.from), b = nodes.get(edge.to);
    const down = b.y > a.y;
    add(edge.from, a.band === b.band || !down ? "top" : "bottom", edge, "a", b.cx);
    add(edge.to, a.band === b.band || down ? "top" : "bottom", edge, "b", a.cx);
  });
  sides.forEach((list) => {
    list.sort((p, q) => p.otherX - q.otherX);
    list.forEach((entry, i) => {
      const node = nodes.get(entry.name);
      const x = node.x + NODE_W * (0.14 + (0.72 * (i + 0.5)) / list.length);
      ports.set(entry.edge, { ...ports.get(entry.edge), [entry.end]: x });
    });
  });
  return ports;
}

function edgeGeometry(a, b, port) {
  const cubic = (x1, y1, cx1, cy1, cx2, cy2, x2, y2) => {
    const t = 0.7, u = 1 - t;
    return {
      d: `M${x1} ${y1} C${cx1} ${cy1} ${cx2} ${cy2} ${x2} ${y2}`,
      lx: u * u * u * x1 + 3 * u * u * t * cx1 + 3 * u * t * t * cx2 + t * t * t * x2,
      ly: u * u * u * y1 + 3 * u * u * t * cy1 + 3 * u * t * t * cy2 + t * t * t * y2,
    };
  };
  if (a.band === b.band) {
    const lift = Math.min(46, 18 + Math.abs(a.x - b.x) / 14);
    return cubic(port.a, a.y, port.a, a.y - lift, port.b, a.y - lift, port.b, a.y - 2);
  }
  const down = b.y > a.y;
  const y1 = down ? a.y + NODE_H : a.y;
  const y2 = down ? b.y - 2 : b.y + NODE_H + 2;
  const mid = (y1 + y2) / 2;
  return cubic(port.a, y1, port.a, mid, port.b, mid, port.b, y2);
}

function backboneEdges(nodes) {
  const quietSources = new Set(["harness", "presentation", "network"]);
  const bySource = new Map();
  state.index.edges.forEach((e) => {
    if (!nodes.has(e.from) || !nodes.has(e.to) || QUIET_TARGETS.has(e.to) || quietSources.has(e.from)) return;
    if (e.from === "game" && nodes.get(e.to).band !== 1) return;
    if (!bySource.has(e.from)) bySource.set(e.from, []);
    bySource.get(e.from).push(e);
  });
  return [...bySource.entries()].flatMap(([source, edges]) =>
    source === "game" ? edges : edges.sort((p, q) => q.calls - p.calls).slice(0, 2));
}

function paintEdges(nodes, visible, mode) {
  const ports = assignPorts(visible, nodes);
  const html = visible.map((e) => {
    const a = nodes.get(e.from), b = nodes.get(e.to);
    const geometry = edgeGeometry(a, b, ports.get(e));
    const width = Math.min(6, 0.8 + Math.log2(e.calls) * 0.55);
    const dir = mode.focus ? (e.from === mode.focus ? "out" : "in") : mode.pinned ? "out" : "";
    const marker = dir === "out" ? "arrow-o" : dir === "in" ? "arrow-i" : "arrow-d";
    const seam = e.seam > e.calls / 2 ? " seam" : "";
    const label = dir
      ? `<g class="edgelabel"><rect x="${geometry.lx - 14}" y="${geometry.ly - 9}" width="28" height="18" rx="4"/><text x="${geometry.lx}" y="${geometry.ly + 4}">${e.calls}</text></g>`
      : "";
    return `<a class="edge-g" href="${edgeHref(e.from, e.to)}" aria-label="${esc(e.from)} calls ${esc(e.to)}, ${e.calls} caller and callee pairs">
      <title>${esc(e.from)} → ${esc(e.to)}: ${e.calls} caller and callee pairs${e.shared ? `, ${e.shared} shared items written` : ""}</title>
      <path class="edge ${dir}${seam}" d="${geometry.d}" stroke-width="${width}" marker-end="url(#${marker})"/>
      <path class="edge-hit" d="${geometry.d}"/>${label}</a>`;
  }).join("");
  $("#map-edges").innerHTML = html;
  const active = mode.focus
    ? new Set([mode.focus, ...visible.flatMap((e) => [e.from, e.to])])
    : mode.pinned ? new Set(mode.pinned) : null;
  document.querySelectorAll(".node").forEach((node) => {
    const name = node.dataset.sub;
    const dim = active ? !active.has(name) : name === "harness" && !state.prefs.showHarnessOnMap;
    node.classList.toggle("off", dim);
  });
}

function drawMap(selection) {
  const nodes = mapLayout();
  const selected = selection.sub || null;
  const pinned = selection.edge || null;
  const showHarness = state.prefs.showHarnessOnMap;
  const height = BANDS.length * (BAND_H + 16) + 20;
  const bands = BANDS.map((label, band) => {
    const y = band * (BAND_H + 16) + 10;
    return `<rect class="band" x="0" y="${y}" width="${MAP_W}" height="${BAND_H}" rx="8"/><text class="bandlabel" x="14" y="${y + 18}">${label}</text>`;
  }).join("");
  const links = (name) => state.index.edges.filter((e) =>
    nodes.has(e.from) && nodes.has(e.to) && (e.from === name || e.to === name) &&
    (showHarness || name === "harness" || (e.from !== "harness" && e.to !== "harness")));
  const rest = () => {
    if (selected) paintEdges(nodes, links(selected), { focus: selected });
    else if (pinned) paintEdges(nodes, state.index.edges.filter((e) => e.from === pinned[0] && e.to === pinned[1]), { pinned });
    else {
      const backbone = backboneEdges(nodes).filter((e) => showHarness || e.from !== "harness");
      paintEdges(nodes, backbone, {});
    }
  };

  const boxes = [...nodes.values()].map((n) => `<a href="${subHref(n.name)}" class="node${n.name === selected ? " sel" : ""}" data-sub="${esc(n.name)}" ${subStyle(n.name)}>
      <foreignObject x="${n.x}" y="${n.y}" width="${NODE_W}" height="${NODE_H}">
        <div xmlns="http://www.w3.org/1999/xhtml" class="nodebox">
          <div class="nodetitle">${esc(n.name)}<span>${n.functions}</span></div>
          <div class="nodeblurb">${esc(n.blurb)}</div>
        </div>
      </foreignObject></a>`).join("");

  const arrow = (id) => `<marker id="${id}" viewBox="0 0 8 8" refX="7" refY="4" markerUnits="userSpaceOnUse" markerWidth="10" markerHeight="10" orient="auto-start-reverse"><path d="M0 0L8 4L0 8z"/></marker>`;
  const tick = state.index.tick;
  const phaseChips = tick.phases
    .map((p) => `<a class="chip" ${subStyle(p.subsystem)} href="${fnHrefById(p.id)}" title="${esc(p.doc)}" data-hover-sub="${esc(p.subsystem)}"><span class="dot"></span>${esc(p.name)}</a>`)
    .join('<span class="arrow">→</span>');

  stage.innerHTML = `
    <div class="tickstrip"><a class="muted" href="${tick.function !== null ? fnHrefById(tick.function) : "#/"}">One game tick:</a>${phaseChips}</div>
    <svg class="map" viewBox="0 0 ${MAP_W} ${height}" role="img" aria-label="Subsystem architecture map">
      <defs>${arrow("arrow-d")}${arrow("arrow-o")}${arrow("arrow-i")}</defs>
      ${bands}<g id="map-edges"></g>${boxes}
    </svg>
    <div class="legend">
      <span><i class="swatch out"></i>calls out</span><span><i class="swatch in"></i>called by</span>
      <span><i class="swatch seam"></i>mostly through seams</span>
      <label><input type="checkbox" id="toggle-harness" ${showHarness ? "checked" : ""}> test harness</label>
      ${selected || pinned ? '<a href="#/">clear selection</a>' : "<span>Showing the backbone. Hover or select a subsystem for all of its links.</span>"}
    </div>`;
  rest();
  if (!selected && !pinned) {
    stage.querySelectorAll("[data-sub], [data-hover-sub]").forEach((element) => {
      const name = element.dataset.sub || element.dataset.hoverSub;
      element.addEventListener("mouseenter", () => paintEdges(nodes, links(name), { focus: name }));
      element.addEventListener("mouseleave", rest);
    });
  }
  $("#toggle-harness").addEventListener("change", (event) => {
    state.prefs.showHarnessOnMap = event.target.checked;
    savePrefs();
    drawMap(selection);
  });
}

async function showMap(selection, token) {
  document.querySelector('[data-nav="map"]').classList.add("on");
  state.current = null;
  drawMap(selection);
  if (selection.sub) {
    const data = await api("sub", { name: selection.sub });
    if (token === state.route) side.innerHTML = subsystemPanel(data);
  } else if (selection.edge) {
    const data = await api("edge", { from: selection.edge[0], to: selection.edge[1] });
    if (token === state.route) side.innerHTML = edgePanel(data);
  } else {
    side.innerHTML = welcomePanel();
  }
}

function welcomePanel() {
  const starts = START_HERE.map(([name, why]) => {
    const id = (state.shortNames.get(name) || [])[0];
    return id === undefined ? "" : `<a href="${fnHrefById(id)}"><b>${esc(name)}</b><span>${esc(why)}</span></a>`;
  }).join("");
  return `<div class="pad">
    <h1 style="font-family:var(--sans)">How the game fits together</h1>
    <p class="muted">Each box is a subsystem. Select one for its interface and state, or an arrow to see exactly which functions cross it and what they pass.</p>
    <h2>Start reading here</h2><div class="start">${starts}</div>
    <h2>Going deeper</h2>
    <ul class="list small">
      <li>Every function has a flow chart and a call graph, with the arguments each call passes.</li>
      <li>Calls through a seam are followed to the real implementation.</li>
      <li>Select a global or a field to see who writes it and who reads it.</li>
      <li>Press <code>/</code> to search names, or paste a <code>0x</code> address from a trace.</li>
    </ul></div>`;
}

function fnRow(entry, right = "") {
  return `<li><span class="dot" ${subStyle(entry.subsystem)}></span><a class="nm" href="${fnHref(entry.name, entry.file)}">${esc(entry.name)}</a>${right ? `<span class="right">${right}</span>` : ""}</li>`;
}

function subsystemPanel(data) {
  const slots = data.slots.map((s) => `<li><div style="min-width:0">
      ${s.function !== null ? `<a class="nm" href="${fnHrefById(s.function)}">${esc(s.key.split("::")[1])}</a>` : `<span class="nm">${esc(s.key.split("::")[1])}</span>`}
      <div class="note">${esc(s.doc)}</div></div></li>`).join("");
  const imports = Object.entries(data.imports).map(([sub, entries]) => `<div style="margin:8px 0">${subChip(sub)}
    <ul class="list">${entries.map((e) => fnRow(e)).join("")}</ul></div>`).join("");
  return `<div class="pad">
    <div class="crumbs">${subChip(data.name)}<span>${data.functions} functions</span>
      <a href="${GITHUB}/blob/${state.index.commit}/docs/subsystems.md#${DOC_SECTIONS.has(data.name) ? esc(data.name) : "the-remaining-subsystems"}" target="_blank" rel="noopener">docs ↗</a></div>
    <p>${esc(data.blurb)}</p>
    ${slots ? `<h2>Interface: seam slots (${data.slots.length})</h2><ul class="list">${slots}</ul>` : ""}
    ${data.entry.length ? `<h2>Most called from outside</h2><ul class="list">${data.entry.map((e) => fnRow(e, `${e.callers} callers`)).join("")}</ul>` : ""}
    ${imports ? `<h2>Depends on</h2>${imports}` : ""}
    ${data.owned.length ? `<h2>State only this subsystem writes (${data.ownedCount})</h2><ul class="list">${data.owned.map((n) => `<li><a class="nm" href="${itemHref(n)}">${esc(n)}</a></li>`).join("")}</ul>` : ""}
    <h2>Files</h2><ul class="list">${data.files.map((f) => `<li><a class="nm" href="${fileHref(f.file)}">${esc(f.file)}</a><span class="right">${f.functions}</span></li>`).join("")}</ul>
  </div>`;
}

function edgePanel(data) {
  const calls = data.calls.map((c) => {
    const args = c.args.map((a) => esc(a.value)).join(", ");
    const via = c.via ? `<span class="tag seam" title="${esc(c.via)}">seam</span>` : c.kinds.includes("dispatch") ? '<span class="tag dispatch">dispatch</span>' : "";
    return `<div class="edgecall"><div class="flowline"><a href="${fnHref(c.caller.name, c.caller.file)}">${esc(c.caller.name)}</a>
      <span class="muted">→</span> <a href="${fnHref(c.callee.name, c.callee.file)}">${esc(c.callee.name)}</a>${args ? `<span class="muted">(${args})</span>` : ""} ${via}</div></div>`;
  }).join("");
  return `<div class="pad">
    <div class="crumbs">${subChip(data.from)}<span>→</span>${subChip(data.to)}</div>
    <h2>Calls across this link (${data.calls.length})</h2>${calls || '<p class="empty">No direct calls.</p>'}
    ${data.shared.length ? `<h2>State both sides write (${data.shared.length})</h2><ul class="list">${data.shared.slice(0, 80).map((n) => (state.itemNames.has(n) ? `<li><a class="nm" href="${itemHref(n)}">${esc(n)}</a></li>` : `<li class="nm">${esc(n)}</li>`)).join("")}</ul>` : ""}
  </div>`;
}

async function showFunction(key, token) {
  const id = state.fnKey.get(key);
  if (id === undefined) throw new Error(`no function ${key}`);
  const fn = await loadFunction(id);
  if (token !== state.route) return;
  if (id === state.index.tick.function) document.querySelector('[data-nav="tick"]').classList.add("on");
  pushTrail({ href: fnHref(fn.name, fn.file), label: fn.name.split("::").pop(), title: fn.name });
  state.current = { fn, redraw: () => drawFunctionStage(fn) };
  drawFunctionStage(fn);
  drawFunctionSide(fn);
}

function drawFunctionStage(fn) {
  const prefs = state.prefs;
  const seam = fn.slot;
  stage.innerHTML = `
    <div class="head">
      <div class="crumbs">${subChip(fn.subsystem)}<a href="${fileHref(fn.file)}">${esc(fn.file)}</a>
        ${fn.address ? `<code class="muted">${esc(fn.address)}</code>` : ""}
        ${seam ? `<span class="tag seam" title="${esc(seam.key)}">seam slot</span>` : ""}
        ${fn.dead ? '<span class="tag dead" title="The code of 3.1c cannot reach this function">dead code</span>' : ""}</div>
      <h1>${esc(fn.name)}</h1>
      ${fn.signature ? `<div class="signature">${esc(fn.signature)}</div>` : ""}
      ${seam?.doc ? `<div class="doc">${esc(seam.doc)}</div>` : ""}
    </div>
    <div class="toolbar">
      <span class="seg"><button data-view="flow" class="${prefs.view === "flow" ? "on" : ""}">Flow chart</button><button data-view="graph" class="${prefs.view === "graph" ? "on" : ""}">Call graph</button></span>
      ${prefs.view === "flow" ? `<label><input type="checkbox" id="pref-statements" ${prefs.statements ? "checked" : ""}> every statement</label>` : ""}
      <label><input type="checkbox" id="pref-harness" ${prefs.harness ? "checked" : ""}> test harness calls</label>
      <span class="muted small">${fn.callers.length} callers, ${fn.callees.length} callees</span>
    </div>
    <div id="fn-view"></div>`;
  stage.querySelectorAll("[data-view]").forEach((button) => button.addEventListener("click", () => {
    prefs.view = button.dataset.view;
    savePrefs();
    drawFunctionStage(fn);
  }));
  $("#pref-harness").addEventListener("change", (event) => {
    prefs.harness = event.target.checked;
    savePrefs();
    drawFunctionStage(fn);
  });
  $("#pref-statements")?.addEventListener("change", (event) => {
    prefs.statements = event.target.checked;
    savePrefs();
    drawFunctionStage(fn);
  });
  const view = $("#fn-view");
  if (prefs.view === "flow") {
    view.innerHTML = flowHtml(fn);
    view.addEventListener("click", onFlowClick);
    view.addEventListener("mouseover", onFlowHover);
  } else {
    drawFunctionGraph(view, fn);
  }
}

function onFlowClick(event) {
  const button = event.target.closest(".ln");
  if (!button) return;
  event.preventDefault();
  event.stopPropagation();
  revealLine(Number(button.dataset.line));
}

function onFlowHover(event) {
  const target = event.target.closest("[data-at]");
  if (target) markLine(Number(target.dataset.at));
}

function visibleNodes(nodes) {
  const prefs = state.prefs;
  const out = [];
  for (const node of nodes) {
    const calls = (node.calls || []).filter((c) => prefs.harness || c.subsystem !== "harness");
    const copy = { ...node, calls };
    if (node.t === "call" && calls.length === 0) {
      if (prefs.statements) out.push({ ...copy, t: "stmt" });
      continue;
    }
    if (node.t === "stmt" || node.t === "break" || node.t === "continue") {
      if (prefs.statements) out.push(copy);
      continue;
    }
    if (node.t === "if") {
      copy.then = visibleNodes(node.then);
      copy.else = visibleNodes(node.else);
      if (!copy.then.length && !copy.else.length && !calls.length) continue;
    } else if (node.t === "loop") {
      copy.body = visibleNodes(node.body);
      if (!copy.body.length && !calls.length) continue;
    } else if (node.t === "switch") {
      copy.cases = node.cases.map((c) => ({ ...c, body: visibleNodes(c.body) })).filter((c) => c.body.length || prefs.statements);
      if (!copy.cases.length && !calls.length) continue;
    }
    out.push(copy);
  }
  return out;
}

function flowHtml(fn) {
  const nodes = visibleNodes(fn.flow);
  if (!nodes.length) return `<p class="empty">This function makes no calls.${state.prefs.statements ? "" : " Turn on “every statement” to see what it does."}</p>`;
  state.callCounts = countCalls(nodes, new Map());
  const notice = fn.flowOk ? "" : '<p class="muted small">This body was too irregular to chart (for example, inline assembly), so these are its calls in source order.</p>';
  return `${notice}<div class="flow">${seqHtml(nodes)}</div>`;
}

function countCalls(nodes, counts) {
  nodes.forEach((node) => {
    (node.calls || []).forEach((call) => counts.set(call.name, (counts.get(call.name) || 0) + 1));
    ["then", "else", "body"].forEach((key) => node[key] && countCalls(node[key], counts));
    (node.cases || []).forEach((c) => countCalls(c.body, counts));
  });
  return counts;
}

const seqHtml = (nodes) => `<div class="seq">${nodes.map(nodeHtml).join("")}</div>`;
const lineButton = (line) => `<button class="ln" data-line="${line}" title="Show line ${line} in the source">${line}</button>`;

function callHtml(call) {
  const plumbing = !call.doc && state.callCounts.get(call.name) >= 3;
  const args = call.args.length && !plumbing
    ? `<div class="args">${call.args.map((a) => `<span class="p">${esc(a.param || "·")}</span><span class="v">${esc(a.value)}</span>`).join("")}</div>`
    : "";
  const via = call.via ? `<span class="tag seam" title="${esc(call.via)}">via seam</span>` : "";
  const sub = call.subsystem ? `<span class="subname">${esc(call.subsystem)}</span>` : "";
  const doc = call.doc ? `<div class="calldoc">${esc(call.doc)}</div>` : "";
  const inner = `<div class="callhead"><span class="callname">${esc(call.name)}</span>${via}${sub}${lineButton(call.line)}</div>${args}${doc}`;
  const style = subStyle(call.subsystem);
  const classes = `call${plumbing ? " plumbing" : ""}`;
  if (call.id === null) return `<div class="${classes}" ${style} data-at="${call.line}">${inner}</div>`;
  return `<a class="${classes}" ${style} data-at="${call.line}" href="${fnHrefById(call.id)}">${inner}</a>`;
}

function condHtml(text, calls) {
  let html = esc(text);
  for (const name of new Set(calls.filter((c) => c.id !== null).map((c) => c.name))) {
    const call = calls.find((c) => c.name === name);
    const short = esc(name.split("::").pop());
    html = html.replace(new RegExp(`\\b${short.replace(/[^\w]/g, "\\$&")}\\b`), `<a href="${fnHrefById(call.id)}">${short}</a>`);
  }
  return html;
}

function nodeHtml(node) {
  switch (node.t) {
    case "call":
      return `<div class="step">${node.calls.map(callHtml).join("")}</div>`;
    case "stmt":
      return `<div class="stmt" data-at="${node.line}">${esc(node.text)}${lineButton(node.line)}</div>`;
    case "return": case "goto": case "break": case "continue": {
      const calls = node.calls.length ? node.calls.map(callHtml).join("") : "";
      return `<div class="step">${calls}<div class="term" data-at="${node.line}"><span class="kw">${node.t}</span><span>${esc(node.text)}</span>${lineButton(node.line)}</div></div>`;
    }
    case "label":
      return `<div class="term label" data-at="${node.line}"><span class="kw">${esc(node.text)}:</span></div>`;
    case "if": {
      const arms = [
        node.then.length ? `<div class="arm">${node.else.length ? '<div class="armlabel">then</div>' : ""}${seqHtml(node.then)}</div>` : "",
        node.else.length ? `<div class="arm"><div class="armlabel">else</div>${seqHtml(node.else)}</div>` : "",
      ].join("");
      return `<div class="branch" data-at="${node.line}"><div class="branchhead"><span class="kw">if</span><span class="cond">${condHtml(node.cond, node.calls)}</span>${lineButton(node.line)}</div>${arms ? `<div class="arms">${arms}</div>` : ""}</div>`;
    }
    case "loop":
      return `<div class="branch loop" data-at="${node.line}"><div class="branchhead"><span class="kw">${node.kind === "do" ? "do…while" : node.kind}</span><span class="cond">${condHtml(node.cond, node.calls)}</span>${lineButton(node.line)}</div><div class="arms"><div class="arm">${seqHtml(node.body)}</div></div></div>`;
    case "switch": {
      const cases = node.cases.map((c) => `<div class="switchcase"><div class="armlabel">${esc(c.labels.join(", "))}</div>${seqHtml(c.body)}</div>`).join("");
      return `<div class="branch" data-at="${node.line}"><div class="branchhead"><span class="kw">switch</span><span class="cond">${condHtml(node.cond, node.calls)}</span>${lineButton(node.line)}</div><div class="arms">${cases}</div></div>`;
    }
    default:
      return "";
  }
}

const shorten = (text, max) => (text.length > max ? `${text.slice(0, max - 1)}…` : text);

function drawColumns(host, spec) {
  const width = Math.max(620, host.clientWidth || 800);
  const boxW = Math.min(300, Math.floor((width - 150) / 3));
  const boxH = 42, gap = 7, top = 26;
  const leftX = 0, centerX = (width - boxW) / 2, rightX = width - boxW;
  const take = (rows, side) => (state.expanded[side] ? rows : rows.slice(0, ROW_CAP));
  const left = take(spec.left, "left"), right = take(spec.right, "right");
  const rowsHigh = Math.max(left.length + (left.length < spec.left.length ? 1 : 0), right.length + (right.length < spec.right.length ? 1 : 0), 1);
  const height = top + rowsHigh * (boxH + gap) + 10;
  const centerY = Math.max(top, Math.min(top + 40, height / 2 - boxH));
  const charsPerBox = Math.floor((boxW - 20) / 7.3);

  const box = (row, x, y, extra = "") => `<a class="gnode ${extra}" href="${row.href}">
    <title>${esc(row.hover || row.title)}</title>
    <rect class="box" x="${x}" y="${y}" width="${boxW}" height="${boxH}" rx="5"/>
    <rect x="${x}" y="${y}" width="3" height="${boxH}" rx="1" style="fill:${color(row.subsystem)};stroke:none"/>
    <text class="title" x="${x + 11}" y="${y + 17}">${esc(shorten(row.title, charsPerBox))}</text>
    <text class="sub" x="${x + 11}" y="${y + 33}">${esc(shorten(row.sub || "", charsPerBox + 3))}</text></a>`;

  const path = (x1, y1, x2, y2, kind) => {
    const mx = (x1 + x2) / 2;
    return `<path class="gedge ${kind}" d="M${x1} ${y1} C${mx} ${y1} ${mx} ${y2} ${x2} ${y2}" marker-end="url(#g-arrow)"/>`;
  };

  let body = "";
  const column = (rows, all, x, sideName, connect) => {
    rows.forEach((row, i) => {
      const y = top + i * (boxH + gap);
      body += connect(y + boxH / 2, row.kind || "");
      body += box(row, x, y);
    });
    if (rows.length < all.length) {
      const y = top + rows.length * (boxH + gap);
      body += `<g class="gmore" data-side="${sideName}"><rect x="${x}" y="${y}" width="${boxW}" height="${boxH}" fill="transparent"/><text x="${x + boxW / 2}" y="${y + 25}" text-anchor="middle">Show ${all.length - rows.length} more</text></g>`;
    }
  };
  column(left, spec.left, leftX, "left", (y, kind) => path(leftX + boxW, y, centerX - 3, centerY + boxH / 2, kind));
  column(right, spec.right, rightX, "right", (y, kind) => path(centerX + boxW, centerY + boxH / 2, rightX - 3, y, kind));
  body += box(spec.center, centerX, centerY, "center");

  host.innerHTML = `<svg class="columns" width="${width}" height="${height}" role="img" aria-label="${esc(spec.center.title)} graph">
    <defs><marker id="g-arrow" viewBox="0 0 8 8" refX="7" refY="4" markerWidth="5" markerHeight="5" orient="auto"><path d="M0 0L8 4L0 8z" fill="var(--line-2)"/></marker></defs>
    <text class="gtitle" x="${leftX}" y="14">${esc(spec.leftLabel)} (${spec.left.length})</text>
    <text class="gtitle" x="${rightX}" y="14">${esc(spec.rightLabel)} (${spec.right.length})</text>
    ${body}</svg>
    <div class="legend">${spec.legend || ""}</div>`;
  host.querySelectorAll(".gmore").forEach((more) => more.addEventListener("click", () => {
    state.expanded[more.dataset.side] = true;
    drawColumns(host, spec);
  }));
}

function argSummary(args) {
  return args.length ? `(${args.map((a) => a.value).join(", ")})` : "";
}

function edgeKind(edge) {
  if (edge.kinds.includes("seam")) return "seam";
  if (edge.kinds.includes("dispatch")) return "dispatch";
  if (edge.kinds.length === 1 && edge.kinds[0] === "ref") return "ref";
  return "";
}

function edgeHover(edge) {
  const lines = [edge.name];
  if (edge.via) lines.push(`via ${edge.via}`);
  if (edge.reason) lines.push(edge.reason);
  edge.args.forEach((a) => lines.push(`${a.param || "·"} = ${a.value}`));
  return lines.join("\n");
}

function drawFunctionGraph(host, fn) {
  const include = (edge) => state.prefs.harness || edge.subsystem !== "harness";
  const row = (edge) => {
    const file = state.index.functions[edge.id][1];
    return {
      href: fnHref(edge.name, file),
      title: edge.name,
      sub: argSummary(edge.args) || edge.subsystem,
      subsystem: edge.subsystem,
      kind: edgeKind(edge),
      hover: edgeHover(edge),
    };
  };
  drawColumns(host, {
    left: fn.callers.filter(include).map(row),
    right: fn.callees.filter(include).map(row),
    center: { href: fnHref(fn.name, fn.file), title: fn.name, sub: fn.subsystem, subsystem: fn.subsystem, hover: fn.signature },
    leftLabel: "Called by",
    rightLabel: "Calls, in source order",
    legend: '<span><i class="swatch"></i>direct</span><span><i class="swatch seam"></i>through a seam</span><span><i class="swatch dispatch"></i>table or vtable dispatch</span><span>Each row shows the arguments passed.</span>',
  });
}

function drawFunctionSide(fn) {
  side.innerHTML = `<div class="tabs">
      <button data-tab="source">Source</button>
      <button data-tab="data">Data (${fn.writes.length} writes, ${fn.reads.length} reads)</button>
    </div><div id="side-body" style="display:flex;flex-direction:column;flex:1;min-height:0"></div>`;
  const select = async (tab) => {
    state.sideTab = tab;
    side.querySelectorAll("[data-tab]").forEach((b) => b.classList.toggle("on", b.dataset.tab === tab));
    if (tab === "source") {
      await openSource($("#side-body"), fn.file, { start: fn.line, end: fn.end, at: fn.line });
    } else {
      state.source = null;
      $("#side-body").innerHTML = dataPanel(fn);
    }
  };
  side.querySelectorAll("[data-tab]").forEach((b) => b.addEventListener("click", () => select(b.dataset.tab)));
  state.current.showSource = () => select("source");
  select(state.sideTab);
}

function dataRows(entries) {
  return entries.map((entry) => `<div class="datarow"><div class="top"><a class="nm" href="${itemHref(entry.name)}">${esc(entry.name)}</a>
      ${entry.access === "maybe" ? '<span class="tag" title="Passed by pointer or reference to a call that may write it">may write</span>' : entry.access === "escape" ? '<span class="tag" title="Its address is taken">address taken</span>' : ""}
      ${entry.class ? `<span class="tag ${esc(entry.class)}">${esc(entry.class)}</span>` : ""}</div>
      ${entry.note ? `<div class="note small">${esc(entry.note)}</div>` : ""}</div>`).join("");
}

function dataPanel(fn) {
  const empty = '<p class="muted small">None recorded.</p>';
  return `<div class="pad">
    <p class="muted small"><span class="tag sim">sim</span> is simulation state that must match 3.1c. <span class="tag">view</span> is presentation only.</p>
    <h2>Writes (${fn.writes.length})</h2>${dataRows(fn.writes) || empty}
    <h2>Reads (${fn.reads.length})</h2>${dataRows(fn.reads) || empty}
    ${fn.pointerAccesses ? `<p class="muted small" style="margin-top:14px">Also ${fn.pointerAccesses} accesses through raw pointers, which have no named target.</p>` : ""}
  </div>`;
}

const KEYWORDS = new Set("alignas asm auto bool break case catch char class const continue default delete do double else enum explicit extern false float for friend goto if inline int long mutable namespace new nullptr operator private protected public register return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while __stdcall __fastcall __cdecl __int64 __try __except __finally".split(" "));
const TOKEN = /(\/\/.*$)|("(?:[^"\\]|\\.)*"?|'(?:[^'\\]|\\.)*'?)|(\b0[xX][0-9a-fA-F]+[uUlL]*|\b\d+(?:\.\d+)?[uUlLfF]*)|([A-Za-z_~]\w*)/g;

function highlightLine(line, blockState, file) {
  let out = "";
  let i = 0;
  let inBlock = blockState.open;
  if (inBlock) {
    const end = line.indexOf("*/");
    if (end < 0) return `<span class="tc">${esc(line)}</span>`;
    out += `<span class="tc">${esc(line.slice(0, end + 2))}</span>`;
    i = end + 2;
    blockState.open = false;
  }
  if (/^\s*#/.test(line) && i === 0) return `<span class="tp">${esc(line)}</span>`;
  let rest = line.slice(i);
  const open = rest.indexOf("/*");
  let tail = "";
  if (open >= 0 && !rest.slice(0, open).includes("//")) {
    const close = rest.indexOf("*/", open + 2);
    if (close < 0) {
      tail = `<span class="tc">${esc(rest.slice(open))}</span>`;
      rest = rest.slice(0, open);
      blockState.open = true;
    }
  }
  let last = 0;
  TOKEN.lastIndex = 0;
  for (let match = TOKEN.exec(rest); match; match = TOKEN.exec(rest)) {
    out += esc(rest.slice(last, match.index));
    last = match.index + match[0].length;
    const text = esc(match[0]);
    if (match[1]) out += `<span class="tc">${text}</span>`;
    else if (match[2]) out += `<span class="ts">${text}</span>`;
    else if (match[3]) out += `<span class="tn">${text}</span>`;
    else out += identifierHtml(match[0], text, file);
  }
  return out + esc(rest.slice(last)) + tail;
}

function identifierHtml(word, text, file) {
  if (KEYWORDS.has(word)) return `<span class="tk">${text}</span>`;
  const ids = state.shortNames.get(word);
  if (ids) {
    const row = state.index.functions[ids.find((id) => state.index.functions[id][1] === file) ?? ids[0]];
    return `<a class="fn" href="${fnHref(row[0], row[1])}">${text}</a>`;
  }
  if (state.typeNames.has(word)) return `<a class="ty" href="${typeHref(word)}">${text}</a>`;
  if (state.itemNames.has(word) && word.startsWith("g_")) return `<a class="gl" href="${itemHref(word)}">${text}</a>`;
  return text;
}

async function openSource(host, path, range) {
  const text = await loadSource(path);
  if (!host.isConnected) return;
  const blockState = { open: false };
  const rows = text.split("\n").map((line, i) => {
    const number = i + 1;
    return `<div class="L" id="L${number}"><i>${number}</i><span class="c0">${highlightLine(line.replace(/\r$/, ""), blockState, path) || " "}</span></div>`;
  });
  const github = `${GITHUB}/blob/${state.index.commit}/${path}#L${range.start}${range.end > range.start ? `-L${range.end}` : ""}`;
  const editor = `vscode://file${state.index.root}/${path}:${range.at || range.start}`;
  host.innerHTML = `<div class="srcbar">
      <a class="path" href="${fileHref(path)}">${esc(path)}</a>
      <span class="muted mono">${range.end > range.start ? `L${range.start}–${range.end}` : `L${range.start}`}</span>
      <span class="links"><a href="${github}" target="_blank" rel="noopener">GitHub ↗</a><a href="${editor}">VS Code ↗</a></span>
    </div><div class="src">${rows.join("")}</div>`;
  const container = host.querySelector(".src");
  state.source = { container, marked: null };
  for (let n = range.start; n <= range.end && n > 0; n++) container.querySelector(`#L${n}`)?.classList.add("in");
  const anchor = container.querySelector(`#L${Math.max(1, (range.at || range.start) - 3)}`);
  if (anchor) container.scrollTop = anchor.offsetTop - container.offsetTop;
}

function markLine(number) {
  const source = state.source;
  if (!source) return null;
  source.marked?.classList.remove("at");
  const row = source.container.querySelector(`#L${number}`);
  row?.classList.add("at");
  source.marked = row;
  return row;
}

async function revealLine(number) {
  if (!state.source && state.current?.showSource) await state.current.showSource();
  const row = markLine(number);
  if (row) {
    const container = state.source.container;
    container.scrollTo({ top: row.offsetTop - container.offsetTop - container.clientHeight / 3, behavior: "smooth" });
  }
}

async function showItem(name, token) {
  const item = await api("item", { name });
  if (token !== state.route) return;
  pushTrail({ href: itemHref(name), label: name.split("::").pop(), title: name });
  state.current = { redraw: () => drawItem(item) };
  drawItem(item);
  const declaration = item.declaration;
  side.innerHTML = '<div id="side-body" style="display:flex;flex-direction:column;flex:1;min-height:0"></div>';
  state.source = null;
  if (declaration) openSource($("#side-body"), declaration.file, { start: declaration.line, end: declaration.line, at: declaration.line });
  else $("#side-body").innerHTML = '<div class="pad muted">No declaration found in the source for this item.</div>';
}

function drawItem(item) {
  const row = (fn, tag) => ({
    href: fnHref(fn.name, fn.file),
    title: fn.name,
    sub: tag || fn.subsystem,
    subsystem: fn.subsystem,
    kind: tag ? "ref" : "",
    hover: `${fn.name}\n${fn.file}`,
  });
  const bySubsystem = (p, q) => p.subsystem.localeCompare(q.subsystem) || p.name.localeCompare(q.name);
  const left = [...[...item.write].sort(bySubsystem).map((f) => row(f)), ...[...item.maybe].sort(bySubsystem).map((f) => row(f, "may write, via a pointer"))];
  const right = [...item.read].sort(bySubsystem).map((f) => row(f));
  const owner = item.owner ? `<a class="chip" href="${typeHref(item.owner)}">in ${esc(item.owner)}</a>` : "";
  stage.innerHTML = `<div class="head">
      <div class="crumbs"><span class="tag">${esc(item.kind)}</span>${owner}${item.class ? `<span class="tag ${esc(item.class)}">${esc(item.class)}</span>` : ""}</div>
      <h1>${esc(item.name)}</h1>
      ${item.declaration?.text || item.ctype ? `<div class="signature">${esc(item.declaration?.text || item.ctype)}</div>` : ""}
      ${item.note ? `<div class="doc">${esc(item.note)}</div>` : ""}
    </div><div id="item-graph" style="margin-top:14px"></div>`;
  drawColumns($("#item-graph"), {
    left,
    right,
    center: { href: itemHref(item.name), title: item.name, sub: item.class || item.kind, subsystem: "", hover: item.note || item.name },
    leftLabel: "Written by",
    rightLabel: "Read by",
    legend: item.escape.length ? `<span>${item.escape.length} more functions take its address, so may touch it.</span>` : "",
  });
}

async function showType(name, token) {
  const type = await api("type", { name });
  if (token !== state.route) return;
  pushTrail({ href: typeHref(name), label: name, title: `struct ${name}` });
  state.current = null;
  const rows = type.fields.map((f) => {
    const key = `${type.name}::${f.name}`;
    const label = state.itemNames.has(key) ? `<a href="${itemHref(key)}">${esc(f.name)}</a>` : esc(f.name);
    return `<tr title="${esc(f.note)}"><td>${label}</td><td class="muted">${esc(f.decl)}</td><td class="n">${f.writes || ""}</td><td class="n">${f.reads || ""}</td><td>${f.class ? `<span class="tag ${esc(f.class)}">${esc(f.class)}</span>` : ""}</td></tr>`;
  }).join("");
  stage.innerHTML = `<div class="head"><div class="crumbs"><span class="tag">type</span><a href="${fileHref(type.file)}">${esc(type.file)}</a></div><h1>${esc(type.name)}</h1></div>
    ${rows ? `<table class="fields"><thead><tr><th>Field</th><th>Declaration</th><th>Writers</th><th>Readers</th><th>Class</th></tr></thead><tbody>${rows}</tbody></table>` : '<p class="empty">No plain fields found.</p>'}`;
  side.innerHTML = '<div id="side-body" style="display:flex;flex-direction:column;flex:1;min-height:0"></div>';
  openSource($("#side-body"), type.file, { start: type.line, end: type.line, at: type.line });
}

async function showFile(path, token) {
  const file = await api("file", { path });
  if (token !== state.route) return;
  state.current = null;
  const rows = file.functions.map((f) => `<li><span class="dot" ${subStyle(f.subsystem)}></span><div style="min-width:0"><a class="nm" href="${fnHref(f.name, f.file)}">${esc(f.name)}</a><div class="note mono small">${esc(f.signature)}</div></div><span class="right">${f.line || ""}</span></li>`).join("");
  stage.innerHTML = `<div class="head"><div class="crumbs"><span class="tag">file</span></div><h1>${esc(path)}</h1></div>
    <h2>Functions (${file.functions.length})</h2><ul class="list">${rows || '<li class="muted">No functions recorded in this file.</li>'}</ul>`;
  side.innerHTML = '<div id="side-body" style="display:flex;flex-direction:column;flex:1;min-height:0"></div>';
  openSource($("#side-body"), path, { start: 1, end: 0, at: 1 });
}

setupSearch();
setupDivider();
init().catch((error) => {
  stage.innerHTML = `<p class="empty">Could not reach the codemap server: ${esc(error.message)}</p>`;
});
