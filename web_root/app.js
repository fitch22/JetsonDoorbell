"use strict";

// Helper function to display upload status
var setStatus = function (text) {
  document.getElementById("el3").innerText = text;
};

var setProgress = function (value) {
  document.getElementById("progressBar").value = value;
};

// When user clicks on a button, trigger file selection dialog
var up_btn = document.getElementById("upload_button");
up_btn.onclick = function (ev) {
  input.click();
};

var form = document.getElementById("play");
form.addEventListener("submit", playSound);
var playDone = true;

async function playSound(event) {
  var url = "/play?name=" + filename;
  event.preventDefault();
  if (playDone) {
    playDone = false;
    fetch(url).then(() => {
      playDone = true;
    });
  }
}

function removeAll(selectBox) {
  while (selectBox.options.length > 0) {
    selectBox.remove(0);
  }
}

function addNameToSelect(name) {
  removeAll(fileSelect);
  listFiles();
  getDefaultFile();
}

// Send a large blob of data chunk by chunk
var sendFileData = function (name, data, chunkSize) {
  var done = false;
  var sendChunk = function (offset) {
    var chunk = data.subarray(offset, offset + chunkSize) || "";
    var opts = { method: "POST", body: chunk };
    var url = "/upload?offset=" + offset + "&file=" + encodeURIComponent(name);
    var ok;
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

var f = "jetson.wav";

// If user selected a file, read it into memory and trigger sendFileData()
var input = document.getElementById("doorbell_file");
input.onchange = function (ev) {
  if (!ev.target.files[0]) return;
  f = ev.target.files[0];
  var r = new FileReader();
  r.readAsArrayBuffer(f);
  r.onload = function () {
    ev.target.value = "";
    sendFileData(f.name, new Uint8Array(r.result), 2048);
  };
};

const defaultFilename = "jetson.wav";
var filename = defaultFilename;
var fileSelect = document.getElementById("select");
var wavfile = document.getElementById("wavfile");
var volume = "80";

var listFiles = function () {
  fetch("/list")
    .then((res) => res.text())
    .then((text) => {
      var names = text.split(",").sort(compareFn);
      for (let i = 0; i < names.length; i++) {
        let opt = document.createElement("option");
        opt.value = names[i];
        opt.innerHTML = names[i];
        fileSelect.append(opt);
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

fileSelect.addEventListener("change", (event) => {
  wavfile.textContent = filename = event.target.value;
  console.log(`Changed tune to ${filename}`);
  var url = "/conf/set?file=" + filename;
  fetch(url).then();
});

var setVolume = document.getElementById("range");
setVolume.addEventListener("change", changeVolume);

function changeVolume(event) {
  const newVolume = event.target.value;
  console.log(`Changed volume to ${newVolume}%`);
  var url = "/conf/set?volume=" + newVolume;
  fetch(url).then();
}

const getDefaultFile = function () {
  fetch("/conf/get/file")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved default File: ", text);
      wavfile.textContent = filename = text;
      fileSelect.value = filename;
    });
};

const getVolume = function () {
  fetch("/conf/get/volume")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved default Volume: ", text);
      setVolume.value = volume = text;
    });
};

listFiles();
getVolume();
