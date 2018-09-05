var myHeading = document.querySelector('h1');
myHeading.textContent = 'Hello World';
//alert('hello world');
//
var config = {
  "localBSURL" : "ws://localhost:8080/socket",
  "BSURL" : "ws://beerpi:8080/socket"
}
//fetch("localhost:8080").then((response) => {
//  const reader = response.body.getReader();
//  const stream = new ReadableStream({
//    start(controller) {
//      controller.enqueue(value


fetch(config.localBSURL)
.then(response => {
  const reader = response.body.getReader();
  return new ReadableStream({
    start(controller) {
      return pump();
      function pump() {
        return reader.read().then(({ done, value }) => {
          // When no more data needs to be consumed, close the stream
          if (done) {
            console.log("Stream Complete");
            controller.close();
            return;
          }
          // Enqueue the next data chunk into our target stream
          controller.enqueue(value);
          console.log("got value");
          return pump();
        });
      }
    }  
  })
})
.then(stream => new Response(stream))
//.then(response => response.blob())
//.then(blob => URL.createObjectURL(blob))
//.then(url => console.log(image.src = url))
.catch(err => console.error(err));
