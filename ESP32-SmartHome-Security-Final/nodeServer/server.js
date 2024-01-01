const path = require('path');
//const express = require('express');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
//const app = express();
//const server = http.createServer();

//const WS_PORT  = 65080;
//const HTTP_PORT = 80;

//const cameraServer = new WebSocket.Server({noServer:true});
//const servoServer = new WebSocket.Server({noServer:true});

const server = http.createServer((req, res) => {
        const {url} = req;
        if(url === '/client.html'){
                const filePath = path.join(__dirname, 'client.html');
                fs.readFile(filePath, 'utf8', (err, data) => {
                        if(err){
                                res.writeHead(500);
                                res.end('Error loading client.html');
                        } else {
                                res.writeHead(200, {'Content-Type': 'text/html'});
                                res.end(data);
                        }
                });
        } else if(url === '/about.html'){
                const filePath = path.join(__dirname, 'about.html');
                fs.readFile(filePath, 'utf8', (err, data) => {
                        if(err){
                                res.writeHead(500);
                                res.end('Error loading client.html');
                        } else {
                                res.writeHead(200, {'Content-Type': 'text/html'});
                                res.end(data);
                        }
                  });
        } else if (url === '/style.css'){
                const filePath = path.join(__dirname, 'style.css');
                fs.readFile(filePath, 'utf8', (err, data) => {
                        if(err){
                                res.writeHead(500);
                                res.end('Error loading style.css');
                        } else {
                                res.writeHead(200, {'Content-Type': 'text/css'});
                                res.end(data);
                        }
                });
        } else {
                res.writeHead(404);
                res.end('Not Found');
        }
});
const WS_PORT  = 65080;

const cameraServer = new WebSocket.Server({noServer:true});
const controlServer = new WebSocket.Server({noServer:true});
let connectedCameraClients = [];
let connectedControlClients = [];

server.on('upgrade', (request, socket, head)=>{
        const pathname = new URL(request.url, `http://${request.headers.host}`).pathname;
        if(pathname === '/camera'){
                cameraServer.handleUpgrade(request, socket, head, (ws) => {
                        cameraServer.emit('connection', ws, request);
                });
        } else if(pathname === '/control'){
                controlServer.handleUpgrade(request, socket, head, (ws) => {
                        controlServer.emit('connection', ws, request);
                });
        } else {
                socket.destroy();
        }
});

//server.listen(WS_PORT, () => console.log(`WebSocket Server listening at ${WS_PORT}`));

    //connectedClients.push(ws);
cameraServer.on('connection', (ws) => {
        connectedCameraClients.push(ws);
        ws.on('message', data => {
                connectedCameraClients.forEach((ws,i)=>{
                        if(ws.readyState === ws.OPEN){
                                ws.send(data);
                        } else {
                                connectedCameraClients.splice(i ,1);
                        }
                })
        });
});

controlServer.on('connection', (ws) => {
        connectedControlClients.push(ws);
        ws.on('message', data => {
                try {
                        const jsonData = JSON.parse(data);

                        connectedControlClients.forEach((ws,i)=>{
                                if(ws.readyState === ws.OPEN){
                                        ws.send(JSON.stringify(jsonData));
                                } else {
                                        connectedControlClients.splice(i ,1);
                                }
                        })
                } catch (error) {
                        console.error('Error parsing JSON data:', error);
                }
        });
});
server.listen(WS_PORT, () => console.log(`WebSocket Server listening at ${WS_PORT}`));
