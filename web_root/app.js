"use strict";

// Helper function to display upload status
const setStatus = function (text) {
  document.getElementById("el3").innerText = text;
};

const setProgress = function (value) {
  document.getElementById("progressBar").value = value;
};

// When user clicks on a button, trigger file selection dialog
const up_btn = document.getElementById("upload_button");
up_btn.onclick = function (ev) {
  input.click();
};

const form = document.getElementById("play");
form.addEventListener("submit", playSound);
let playDone = true;

async function playSound(event) {
  const url = "/play?name=" + filename1 + "&vol=1";
  event.preventDefault();
  if (playDone) {
    playDone = false;
    fetch(url).then(() => {
      playDone = true;
    });
  }
}

const form2 = document.getElementById("play2");
form2.addEventListener("submit", playSound2);
let playDone2 = true;

async function playSound2(event) {
  const url = "/play?name=" + filename2 + "&vol=2";
  event.preventDefault();
  if (playDone2) {
    playDone2 = false;
    fetch(url).then(() => {
      playDone2 = true;
    });
  }
}

function removeAll(selectBox) {
  while (selectBox.options.length > 0) {
    selectBox.remove(0);
  }
}

function addNameToSelect(name) {
  listFiles();
}

// Send a large blob of data chunk by chunk
const sendFileData = function (name, data, chunkSize) {
  let done = false;
  const sendChunk = function (offset) {
    const chunk = data.subarray(offset, offset + chunkSize) || "";
    const opts = { method: "POST", body: chunk };
    const url = "/upload?offset=" + offset + "&file=" + encodeURIComponent(name);
    let ok;
    setStatus(
      "Uploading " +
        name +
        ", bytes " +
        offset +
        ".." +
        (offset + chunk.length) +
        " of " +
        data.length
    );
    fetch(url, opts)
      .then(function (res) {
        if (res.ok && chunk.length > 0) {
          sendChunk(offset + chunk.length);
          setProgress((offset + chunk.length) / data.length);
        } else {
          done = true;
        }
        ok = res.ok;
        return res.text();
      })
      .then(function (text) {
        if (!ok) {
          setStatus("Error: " + text);
        }
      })
      .finally(function () {
        if (done && ok) {
          addNameToSelect(name);
          console.log("Updating file list");
        }
      });
  };
  sendChunk(0);
};

let f = "jetson.wav";

// If user selected a file, read it into memory and trigger sendFileData()
const input = document.getElementById("doorbell_file");
input.onchange = function (ev) {
  if (!ev.target.files[0]) return;
  f = ev.target.files[0];
  const r = new FileReader();
  r.readAsArrayBuffer(f);
  r.onload = function () {
    ev.target.value = "";
    sendFileData(f.name, new Uint8Array(r.result), 2048);
  };
};

const defaultFilename = "jetson.wav";
let filename1 = defaultFilename;
let filename2 = defaultFilename;
const fileSelect = document.getElementById("select");
const fileSelect2 = document.getElementById("select2");
let volume = "-20";
let volume2 = "-20";

const listFiles = function () {
  fetch("/list")
    .then((res) => res.text())
    .then((text) => {
      removeAll(fileSelect);
      removeAll(fileSelect2);
      const names = text.split(",").sort(compareFn);
      for (let i = 0; i < names.length; i++) {
        let opt = document.createElement("option");
        opt.value = names[i];
        opt.innerHTML = names[i];
        fileSelect.append(opt);
      }
      for (let i = 0; i < names.length; i++) {
        let opt2 = document.createElement("option");
        opt2.value = names[i];
        opt2.innerHTML = names[i];
        fileSelect2.append(opt2);
      }
      getDefaultFile();
    });
};

function compareFn(a, b) {
  if (a.toUpperCase() < b.toUpperCase()) {
    return -1;
  } else if (a.toUpperCase() > b.toUpperCase()) {
    return 1;
  } else return 0;
}

function flashSaved() {
  const el = document.getElementById("save_status");
  el.style.visibility = "visible";
  setTimeout(() => { el.style.visibility = "hidden"; }, 1500);
}

fileSelect.addEventListener("change", (event) => {
  filename1 = event.target.value;
  console.log(`Changed Doorbell 1 tune to ${filename1}`);
  const url = "/conf/set?file1=" + filename1;
  fetch(url).then(() => flashSaved());
});

fileSelect2.addEventListener("change", (event) => {
  filename2 = event.target.value;
  console.log(`Changed Doorbell 2 tune to ${filename2}`);
  const url = "/conf/set?file2=" + filename2;
  fetch(url).then(() => flashSaved());
});

const setVolume = document.getElementById("range");
setVolume.addEventListener("change", changeVolume);
setVolume.addEventListener("input", (e) => {
  document.getElementById("vol1_out").value = e.target.value;
});

function changeVolume(event) {
  const newVolume = event.target.value;
  console.log(`Changed Doorbell 1 volume to ${newVolume}dB`);
  const url = "/conf/set?volume1=" + newVolume;
  fetch(url).then(() => flashSaved());
}

const setVolume2 = document.getElementById("range2");
setVolume2.addEventListener("change", changeVolume2);
setVolume2.addEventListener("input", (e) => {
  document.getElementById("vol2_out").value = e.target.value;
});

function changeVolume2(event) {
  const newVolume = event.target.value;
  console.log(`Changed Doorbell 2 volume to ${newVolume}dB`);
  const url = "/conf/set?volume2=" + newVolume;
  fetch(url).then(() => flashSaved());
}

const getDefaultFile = function () {
  fetch("/conf/get/file1")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 1 tune: ", text);
      fileSelect.value = filename1 = text;
    });
  fetch("/conf/get/file2")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 2 tune: ", text);
      fileSelect2.value = filename2 = text;
    });
};

const getVolume = function () {
  fetch("/conf/get/volume1")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 1 Volume: ", text, "dB");
      setVolume.value = volume = text;
      document.getElementById("vol1_out").value = text;
    });
  fetch("/conf/get/volume2")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 2 Volume: ", text, "dB");
      setVolume2.value = volume2 = text;
      document.getElementById("vol2_out").value = text;
    });
};

listFiles();
getVolume();
