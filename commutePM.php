<?php

// First, include Requests
include('libRequests/Requests.php');

// Next, make sure Requests can load internal classes
Requests::register_autoloader();

// Now let's make a request!
$request = Requests::get('https://maps.googleapis.com/maps/api/distancematrix/json?units=imperial&origins=YOUR_WORK_ADDRESS&destinations=YOUR_HOME_ADDRESS&key=YOUR_API_KEY');

// Check what we received
//var_dump($request);

// get the body of the response and JSON decode
$input = json_decode($request->body, TRUE);
//var_dump($input);
//var_dump($input["rows"][0]["elements"][0]["duration"]["value"]);

// pull out the commute duration in seconds
echo $input["rows"][0]["elements"][0]["duration"]["value"];

?>
