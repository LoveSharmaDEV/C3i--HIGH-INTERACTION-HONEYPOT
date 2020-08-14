var mysql = require('mysql');
var http = require('http');


http.createServer(function (req, res) {
    res.write('http Server');
    res.end();
  }).listen(8080);

var myArgs = process.argv.slice(2)


http_db_host = myArgs[0];
http_db_user = myArgs[1];
http_db_pass = myArgs[2];


var con = mysql.createConnection({
  host: http_db_host,
  user: http_db_user,
  password: http_db_pass
});

con.connect(function(err) {
  if (err) throw err;
  console.log("Connected!");
});
