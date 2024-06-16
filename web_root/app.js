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
  var url = "/play?name=" + filename1 + "&vol=1";
  event.preventDefault();
  if (playDone) {
    playDone = false;
    fetch(url).then(() => {
      playDone = true;
    });
  }
}

var form2 = document.getElementById("play2");
form2.addEventListener("submit", playSound2);
var playDone2 = true;

async function playSound2(event) {
  var url = "/play?name=" + filename2 + "&vol=2";
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
var filename1 = defaultFilename;
var filename2 = defaultFilename;
var fileSelect = document.getElementById("select");
var fileSelect2 = document.getElementById("select2");
//var wavfile = document.getElementById("wavfile");
var volume = "-20";
var volume2 = "-20";

var listFiles = function () {
  fetch("/list")
    .then((res) => res.text())
    .then((text) => {
      //var names = ["bob", "nancy", "tom"]; //text.split(",").sort(compareFn);
      var names = text.split(",").sort(compareFn);
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

fileSelect.addEventListener("change", (event) => {
  filename1 = event.target.value;
  console.log(`Changed Doorbell 1 tune to ${filename1}`);
  var url = "/conf/set?file1=" + filename1;
  fetch(url).then();
});

fileSelect2.addEventListener("change", (event) => {
  filename2 = event.target.value;
  console.log(`Changed Doorbell 2 tune to ${filename2}`);
  var url = "/conf/set?file2=" + filename2;
  fetch(url).then();
});

var setVolume = document.getElementById("range");
setVolume.addEventListener("change", changeVolume);

function changeVolume(event) {
  const newVolume = event.target.value;
  console.log(`Changed Doorbell 1 volume to ${newVolume}dB`);
  var url = "/conf/set?volume1=" + newVolume;
  fetch(url).then();
}

var setVolume2 = document.getElementById("range2");
setVolume2.addEventListener("change", changeVolume2);

function changeVolume2(event) {
  const newVolume = event.target.value;
  console.log(`Changed Doorbell 2 volume to ${newVolume}dB`);
  var url = "/conf/set?volume2=" + newVolume;
  fetch(url).then();
}

const getDefaultFile = function () {
  fetch("/conf/get/file1")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 1 tune: ", text);
      //wavfile.textContent = filename = text;
      fileSelect.value = filename1 = text;
    });
  fetch("/conf/get/file2")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 2 tune: ", text);
      //wavfile.textContent = filename = text;
      fileSelect2.value = filename2 = text;
    });
};

const getVolume = function () {
  fetch("/conf/get/volume1")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 1 Volume: ", text, "dB");
      setVolume.value = volume = text;
    });
  fetch("/conf/get/volume2")
    .then((res) => res.text())
    .then((text) => {
      console.log("Retrieved Doorbell 2 Volume: ", text, "dB");
      setVolume2.value = volume2 = text;
    });
};

listFiles();
getVolume();
