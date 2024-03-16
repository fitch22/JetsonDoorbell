"use strict";

var f; // filename

const fw_file = document.getElementById("fw_file"); // input (hidden)
const fw_select = document.getElementById("fw_select"); // file select button
const fw_upload = document.getElementById("fw_upload"); // upload button
const filename = document.getElementById("filename"); // filename

// Helper function to display upload status
var setStatus = function (text) {
  document.getElementById("filename").innerText = text;
};

var setProgress = function (value) {
  document.getElementById("progressBar").value = value;
};

// Add listener for the file select button
fw_select.addEventListener(
  "click",
  (e) => {
    console.log("Someone clicked fw_select");
    if (fw_file) {
      fw_file.click();
    }
  },
  false
);

// Add listener for the upload button
fw_upload.addEventListener("click", function (event) {
  event.preventDefault();
  if (f) {
    console.log("Upload button pushed, attempting to upload file: ", f.name);
    var r = new FileReader();
    r.readAsArrayBuffer(f);
    r.onload = function () {
      event.target.value = "";
      sendFileData(f.name, new Uint8Array(r.result), 2048);
      console.log(f.name, r.result);
    };
  }
});

fw_file.onchange = function (e) {
  if (!e.target.files[0]) return;
  f = e.target.files[0];
  filename.innerText = f.name;
  console.log("Selected file: ", f.name);
  //  };
};

// up_btn.onclick = function (ev) {
//   up_btn.console.log("Upload button pushed");
//   //uploadMe.click();
// };

// Send a large blob of data chunk by chunk
var sendFileData = function (name, data, chunkSize) {
  var done = false;
  var sendChunk = function (offset) {
    var chunk = data.subarray(offset, offset + chunkSize) || "";
    var opts = { method: "POST", body: chunk };
    var url =
      "/firmware/upload?offset=" +
      offset +
      "&total=" +
      data.length +
      "&name=" +
      encodeURIComponent(name);
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
          console.log("Firmware upload complete");
          getFirmwareInfo();
        }
      });
  };
  sendChunk(0);
};

var getFirmwareInfo = function () {
  fetch("/firmware/info")
    .then((res) => res.text())
    .then((text) => {
      var info = text.split(",");
      document.getElementById("project").innerText = info[0];
      document.getElementById("datetime").innerText = info[1];
      document.getElementById("version").innerText = info[2];
      document.getElementById("label").innerText = info[3];
    });
};

getFirmwareInfo();
