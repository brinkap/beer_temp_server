var webSocketFactory = {
  connectionTries: 1,
  connect: function(url) {
    var ws = new WebSocket(url, );
    ws.onopen = function(ev) {
      console.log('Connection opened.');
    }
    ws.onmessage = function(ev) {
      console.log('Response from server: ' + ev.data);
    }
    ws.onclose = function(ev) {
      console.log('Connection closed. code:', ev);
    }
    ws.onerror = function(event) {
      console.log('An error occurred. event:', event);
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

var myButton = document.querySelector("button[name=connect_button]");
myButton.onclick = function(){
  var webSocket = webSocketFactory.connect("ws://localhost:8000/");
}
//var sock = new WebSocket("ws:localhost:8080");
//console.log("Web socket connection " + webSocket.readyState)

//webSocket.onmessage = function(event) {
//  console.log(event.data)
//}

