// MIDI Mixer runs this file as a Node.js process. It only keeps VolumeOsd.exe
// running while MIDI Mixer is open; the OSD itself watches Windows audio sessions.
//
// Talks to MIDI Mixer over the same Node IPC protocol as the official
// midi-mixer-plugin SDK ([channel, replyId, args]) to avoid shipping node_modules.

const { spawn } = require("child_process");
const path = require("path");

const exe = path.join(__dirname, "bin", "VolumeOsd.exe");

const send = (channel, ...args) => {
  if (process.send) process.send([channel, null, args]);
};

const log = (level, message) => send("plugin-log", level, message);
const setStatus = (text) => send("plugin-setSettingsStatus", "status", text);

let child = null;

const start = () => {
  child = spawn(exe, ["--parent", String(process.pid)], { stdio: "ignore" });
  setStatus("Працює");

  child.on("error", (err) => {
    log("error", `Не вдалося запустити ${exe}: ${err.message}`);
    setStatus("Помилка: VolumeOsd.exe не знайдено");
  });

  child.on("exit", (code) => {
    log("info", `VolumeOsd завершився з кодом ${code}`);
    setStatus("Зупинено");
  });
};

const stop = () => {
  if (child && child.exitCode === null) child.kill();
};

process.on("message", (message) => {
  if (Array.isArray(message) && message[0] === "plugin-close") {
    stop();
    send("plugin-close");
  }
});

process.on("exit", stop);

// MIDI Mixer shows these in the plugin list, same as the SDK does
setInterval(() => {
  send("plugin-stats", {
    ram: Math.round(process.memoryUsage().rss / 1024),
    cpu: 0,
    runTime: process.uptime() * 1000,
  });
}, 5000);

start();
