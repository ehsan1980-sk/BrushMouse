/*
	wroom tcpサーバ兼webサーバ
	webサーバは8000番
	tcpサーバは11111
*/
var fs = require('fs');
var http = require('http');
var server = http.createServer();
var url = require('url');


// サーバ処理(html送信)
server.on('request', function(req, res) {
	var urlInfo = url.parse(req.url, true);
  	var pathname = urlInfo.pathname;
  	//console.log(urlInfo);
  	console.log(pathname);

  	var stream = fs.createReadStream('sp.html');
  	res.writeHead(200, {'Content-Type': 'text/html; charset=utf-8'});
  	stream.pipe(res);
});

var io = require('socket.io').listen(server); // 同じ8000番portとのsocket通信ということ
server.listen(8000);
console.log('Listening Start on webSOcket Server - ' + "192.168.179.3" + ':' + 8000);

// webソケット
io.sockets.on('connection', function(socket) {
	// callback はnodejs 側で実行される!!
  /*
	socket.emit('greeting', {message: 'hello'}, function (data) {  
		console.log('webclient connected: ' + data);
	});
  */
  console.log("conected http client");
	socket.on('message', function(data) {
		var len = data.length;
		var buffer = new Buffer(len + 1);
		for(var index = 0; index < len; index++)
		{
			buffer.writeUInt8(data[index], index);
			//console.log(data[index]);
		}
		buffer.writeUInt8(0, len);
		

		sendAllNetClient(buffer); // netClientに転送
	});

	socket.on('disconnect', function() {
		console.log("disconnect web socket");
  	});
});

// tcpソケット
var net = require('net');

var net_server = net.createServer();
net_server.maxConnections = 3;

function sendAllHttpClient(d){
	io.sockets.emit("message", d);
}

function sendAllNetClient(d){
    //console.log("writeData: " + d[0] + ":" + d[1] + ":" + d[2]);

	for(var i in clients){
		clients[i].writeData(d);
	}
}

function Client(socket) {
  this.socket = socket;
}

Client.prototype.writeData = function(d) {
  var socket = this.socket;
  if (socket.writable) {
    var key = socket.remoteAddress + ':' + socket.remotePort;
    //process.stdout.write('[' + key + '] - ' + d);
    socket.write(d);
  }
};

var clients = {};

function chomp(raw_text)
{
  return raw_text.replace(/(\n|\r)+$/,'');
}

net_server.on('connection', function(socket) {
	//	net_server.getConnections(function(err, count){});
  var status = net_server.connections + '/' + net_server.maxConnections;
  var key = socket.remoteAddress + ':' + socket.remotePort;
  console.log('Net Connection Start(' + status + ') - ' + key);
  clients[key] = new Client(socket);

  var data = '';
  var newline = /\r\n|\n/;
  socket.on('data', function(chunk) {
    //var k = chunk.toString();
    /*
    if (newline.test(k)) {
      k = chomp(k);
      clients['127.0.0.1:' + k].writeData('test data\n'); // socketではなくsocketをもつclientオブジェクトを取得するため複雑になっている
      k ='';
    }
    */
    console.log("data from wroom:");
    let dataArr = [];
    for(let index in chunk)
    {
      console.log(chunk[index]);
      if(index < 15)
        dataArr.push(chunk[index]);
      else
        break;
    }
    sendAllHttpClient(dataArr);
  });

  // エラーキャッチ
  socket.on("error",function(err){
    console.log("net server socket error: ");
    console.log(err);
  });

  socket.on('end', function(socket) {
    console.log('Connection End(' + status + ') - ' + key);
    delete clients[key];
  });
});

net_server.on('close', function() {
  console.log('Net Server Closed');
});

// ctrl-C
process.on('SIGINT', function() {
  for (var i in clients) {
    var socket = clients[i].socket;
    socket.end();
  }
  net_server.close();

  /*server.clients.forEach(function(client) {
    client.close();
  });*/
  io.engine.close();
  server.close();

  process.exit();
});


net_server.on('listening', function() {
  var addr = net_server.address();
  console.log('Listening Start on Net Server - ' + addr.address + ':' + addr.port);
});

// "localhost" では外部からアクセスできない模様。原因調べたい
//net_server.listen(11111, "192.168.179.3");
net_server.listen(11111, "192.168.179.3");
// vim: set sw=2 sts=2 et :
