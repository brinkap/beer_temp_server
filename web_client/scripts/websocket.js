/*var webSocketFactory = {
  connectionTries: 2,
  connect: function(url) {
  var ws = new WebSocket(url, );
  ws.onopen = function(ev) {
  console.log('Connection opened.');
  connectionHeading.textContent = "Connection Status is OPEN";
  }
  ws.onmessage = function(ev) {
  console.log('Response from server: ' + ev.data);
  }
  ws.onclose = function(ev) {
  console.log('Connection closed. code:', ev);
  connectionHeading.textContent = "Connection Status is CLOSED";
  }
  ws.onerror = function(event) {
  console.log('An error occurred. event:', event);
  connectionHeading.textContent = "Connection Status is CLOSED";
  }

  ws.addEventListener("error", e => {
// readyState === 3 is CLOSED
if (e.target.readyState === 3) {
this.connectionTries--;

if (this.connectionTries > 0) {
setTimeout(() => this.connect(url), 5000);
} else {
throw new Error("Maximum number of connection trials has been reached");
}

}
});
}
};

*/
var connectionHeading = document.querySelector("p[name=connection_status]")
var myButton = document.querySelector("button[name=connect_button]");
//webSocket = webSocketFactory.connect("ws://localhost:8000/");
connectionHeading.textContent = "Connection Status is ATTEMPTING";
var url = "ws://localhost:7681/";
var webSocket = new WebSocket(url);
webSocket.onopen = function(ev) {
  console.log('Connection opened.');
  connectionHeading.textContent = "Connection Status is OPEN";
  localStorage.setItem('websocket', webSocket);
  console.log("adding websocket localStorage")
  webSocket.send("S,50");
}
webSocket.onmessage = function(ev) {
  console.log('Response from server: ' + ev.data);
}
webSocket.onclose = function(ev) {
  console.log('Connection closed. code:', ev);
  connectionHeading.textContent = "Connection Status is CLOSED";
  localStorage.removeItem('websocket');
  console.log("removing websocket localStorage")
}

var dcButton = document.querySelector("button[name=disconnect_button]");
dcButton.onclick = function(){
  if(!localStorage.getItem('websocket')){
    console.log("webSocket not yet connected, nothing to do");
  }else {
    var webSocket = localStorage.getItem('websocket');
    if(webSocket.readyState !== 3){
      console.log("disconnecting from active socket now");
      webSocket.close();
    }else{
      console.log("webSocket already disconnected, nothing to do");
    }
  }
}
//var sock = new WebSocket("ws:localhost:8080");
//console.log("Web socket connection " + webSocket.readyState)

//webSocket.onmessage = function(event) {
//  console.log(event.data)
//}

