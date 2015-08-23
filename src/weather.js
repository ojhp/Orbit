function requestWeather() {
    navigator.geolocation.getCurrentPosition(
        function(pos) { getWeather(pos.coords.latitude, pos.coords.longitude); },
        function(err) { console.log('Error finding position: ' + err); },
        { timeout: 15000, maximumAge: 60000 });
}

function getWeather(latitude, longitude) {
    console.log('Fetching weather for (' + latitude + ', ' + longitude + ')');
    
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + latitude + '&lon=' + longitude;
    
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        sendWeather(JSON.parse(this.responseText));
    };
    xhr.open('GET', url);
    xhr.send();
}

function sendWeather(data) {
    var temperature = data.main.temp;
    var conditions = data.weather[0].main;
    
    console.log('Got temperature = ' + temperature + ' & conditions = ' + conditions);
    
    var msg = {
        'KEY_TEMPERATURE': temperature,
        'KEY_CONDITIONS': conditions
    };
    
    Pebble.sendAppMessage(msg,
        function(e) { console.log('Weather info sent'); },
        function(e) { console.log('Failed to send weather info'); });
}

Pebble.addEventListener('ready', function(e) {
    console.log('PebbleKit.js ready');
});

Pebble.addEventListener('appmessage', function(e) {
    console.log('Weather requested');
    requestWeather();
});