"use strict";

let f; // filename

const fw_file = document.getElementById("fw_file"); // input (hidden)
const fw_select = document.getElementById("fw_select"); // file select button
const fw_upload = document.getElementById("fw_upload"); // upload button
const filename = document.getElementById("filename"); // filename

// Helper function to display upload status
const setStatus = function (text) {
  document.getElementById("filename").innerText = text;
};

const setProgress = function (value) {
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
    const r = new FileReader();
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
  fw_upload.disabled = false;
  console.log("Selected file: ", f.name);
};

// Send a large blob of data chunk by chunk
const sendFileData = function (name, data, chunkSize) {
  let done = false;
  const sendChunk = function (offset) {
    const chunk = data.subarray(offset, offset + chunkSize) || "";
    const opts = { method: "POST", body: chunk };
    const url =
      "/firmware/upload?offset=" +
      offset +
      "&total=" +
      data.length +
      "&name=" +
      encodeURIComponent(name);
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
          console.log("Firmware upload complete");
          getFirmwareInfo();
        }
      });
  };
  sendChunk(0);
};

const getFirmwareInfo = function () {
  fetch("/firmware/info")
    .then((res) => res.text())
    .then((text) => {
      const info = text.split(",");
      document.getElementById("project").innerText = info[0];
      document.getElementById("datetime").innerText = info[1];
      document.getElementById("version").innerText = info[2];
      document.getElementById("label").innerText = info[3];
    });
};

getFirmwareInfo();
