var myButton = document.querySelector("button[name=name_button]");
var myHeading = document.querySelector("h2");

function setUserName(){
  var myName = prompt('Please enter your name.');
  localStorage.setItem('name', myName);
  myHeading.textContent = 'User set name to ' + myName;
  console.log("User name changed to " + myName);
}

if(!localStorage.getItem('name')) {
  setUserName();
}else{
  var storedName = localStorage.getItem('name');
  myHeading.textContent = 'User set name to ' + storedName;
}
myButton.onclick = function(){
  setUserName();
}
