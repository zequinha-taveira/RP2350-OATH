/**
 * WebSocket Server para WebUSB
 * Servidor de notificações em tempo real para RP2350-OATH
 * 
 * Instalação: npm install ws express cors
 * Uso: node websocket_server.js
 */

const WebSocket = require('ws');
const express = require('express');
const cors = require('cors');
const http = require('http');

class WebUSBWebSocketServer {
    constructor(port = 8080) {
        this.port = port;
        this.app = express();
        this.server = http.createServer(this.app);
        this.wss = new WebSocket.Server({ server: this.server });
        
        // Device connections
        this.devices = new Map(); // deviceId -> WebSocket
        this.clients = new Map(); // clientId -> WebSocket
        
        this.setupMiddleware();
        this.setupRoutes();
        this.setupWebSocket();
    }
    
    setupMiddleware() {
        this.app.use(cors());
        this.app.use(express.json());
    }
    
    setupRoutes() {
        // Health check
        this.app.get('/health', (req, res) => {
            res.json({ 
                status: 'ok', 
                timestamp: new Date().toISOString(),
                devices: this.devices.size,
                clients: this.clients.size
            });
        });
        
        // Device registration
        this.app.post('/api/devices/register', (req, res) => {
            const { deviceId, metadata } = req.body;
            
            if (!deviceId) {
                return res.status(400).json({ error: 'Device ID required' });
            }
            
            // Store device info
            const deviceInfo = {
                id: deviceId,
                metadata: metadata || {},
                registeredAt: new Date().toISOString(),
                lastSeen: new Date().toISOString()
            };
            
            res.json({ 
                success: true, 
                device: deviceInfo,
                wsUrl: `ws://localhost:${this.port}/ws`
            });
        });
        
        // Get active devices
        this.app.get('/api/devices', (req, res) => {
            const devices = Array.from(this.devices.keys()).map(id => ({
                id,
                lastSeen: this.devices.get(id).lastSeen
            }));
            res.json({ devices });
        });
        
        // Send notification to device
        this.app.post('/api/devices/:id/notify', (req, res) => {
            const deviceId = req.params.id;
            const { event, data } = req.body;
            
            const device = this.devices.get(deviceId);
            if (device && device.ws.readyState === WebSocket.OPEN) {
                device.ws.send(JSON.stringify({
                    type: 'notification',
                    event,
                    data,
                    timestamp: new Date().toISOString()
                }));
                
                res.json({ success: true, sent: true });
            } else {
                res.status(404).json({ success: false, error: 'Device not connected' });
            }
        });
        
        // Broadcast to all clients
        this.app.post('/api/broadcast', (req, res) => {
            const { event, data } = req.body;
            
            const message = JSON.stringify({
                type: 'broadcast',
                event,
                data,
                timestamp: new Date().toISOString()
            });
            
            let sent = 0;
            this.clients.forEach(client => {
                if (client.readyState === WebSocket.OPEN) {
                    client.send(message);
                    sent++;
                }
            });
            
            res.json({ success: true, sent, recipients: this.clients.size });
        });
    }
    
    setupWebSocket() {
        this.wss.on('connection', (ws, req) => {
            const url = new URL(req.url, `http://${req.headers.host}`);
            const clientId = url.searchParams.get('clientId');
            const deviceId = url.searchParams.get('deviceId');
            const type = url.searchParams.get('type') || 'client';
            
            console.log(`[${new Date().toISOString()}] New connection: ${type} - ${clientId || deviceId}`);
            
            if (type === 'device' && deviceId) {
                // Device connection
                this.devices.set(deviceId, { 
                    ws, 
                    lastSeen: new Date().toISOString(),
                    clientId: clientId || 'unknown'
                });
                
                // Notify all clients
                this.broadcast({
                    type: 'device_connected',
                    deviceId,
                    timestamp: new Date().toISOString()
                });
                
                ws.on('message', (data) => {
                    try {
                        const message = JSON.parse(data);
                        this.handleDeviceMessage(deviceId, message);
                    } catch (e) {
                        console.error('Invalid device message:', e);
                    }
                });
                
                ws.on('close', () => {
                    console.log(`Device ${deviceId} disconnected`);
                    this.devices.delete(deviceId);
                    this.broadcast({
                        type: 'device_disconnected',
                        deviceId,
                        timestamp: new Date().toISOString()
                    });
                });
                
            } else {
                // Client connection
                const id = clientId || `client_${Date.now()}`;
                this.clients.set(id, { ws, lastSeen: new Date().toISOString() });
                
                ws.on('message', (data) => {
                    try {
                        const message = JSON.parse(data);
                        this.handleClientMessage(id, message);
                    } catch (e) {
                        console.error('Invalid client message:', e);
                    }
                });
                
                ws.on('close', () => {
                    console.log(`Client ${id} disconnected`);
                    this.clients.delete(id);
                });
                
                // Send welcome message
                ws.send(JSON.stringify({
                    type: 'welcome',
                    clientId: id,
                    devices: Array.from(this.devices.keys()),
                    timestamp: new Date().toISOString()
                }));
            }
            
            // Heartbeat
            const heartbeat = setInterval(() => {
                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ type: 'ping', timestamp: Date.now() }));
                } else {
                    clearInterval(heartbeat);
                }
            }, 30000);
            
            ws.on('close', () => clearInterval(heartbeat));
        });
        
        console.log(`WebSocket Server ready on port ${this.port}`);
    }
    
    handleDeviceMessage(deviceId, message) {
        console.log(`[${deviceId}] Message:`, message);
        
        // Forward to clients or process
        switch (message.type) {
            case 'credential_added':
                this.broadcast({
                    type: 'credential_added',
                    deviceId,
                    credential: message.credential,
                    timestamp: new Date().toISOString()
                });
                break;
                
            case 'credential_removed':
                this.broadcast({
                    type: 'credential_removed',
                    deviceId,
                    credentialId: message.credentialId,
                    timestamp: new Date().toISOString()
                });
                break;
                
            case 'config_changed':
                this.broadcast({
                    type: 'config_changed',
                    deviceId,
                    config: message.config,
                    timestamp: new Date().toISOString()
                });
                break;
                
            case 'security_event':
                this.broadcast({
                    type: 'security_event',
                    deviceId,
                    event: message.event,
                    severity: message.severity,
                    timestamp: new Date().toISOString()
                });
                break;
                
            case 'error':
                this.broadcast({
                    type: 'error',
                    deviceId,
                    error: message.error,
                    timestamp: new Date().toISOString()
                });
                break;
        }
    }
    
    handleClientMessage(clientId, message) {
        console.log(`[${clientId}] Message:`, message);
        
        switch (message.type) {
            case 'get_devices':
                this.sendToClient(clientId, {
                    type: 'devices_list',
                    devices: Array.from(this.devices.keys()),
                    timestamp: new Date().toISOString()
                });
                break;
                
            case 'send_to_device':
                const device = this.devices.get(message.deviceId);
                if (device && device.ws.readyState === WebSocket.OPEN) {
                    device.ws.send(JSON.stringify({
                        type: 'client_command',
                        clientId,
                        command: message.command,
                        data: message.data,
                        timestamp: new Date().toISOString()
                    }));
                    
                    this.sendToClient(clientId, {
                        type: 'command_sent',
                        deviceId: message.deviceId,
                        timestamp: new Date().toISOString()
                    });
                } else {
                    this.sendToClient(clientId, {
                        type: 'error',
                        error: 'Device not connected',
                        deviceId: message.deviceId,
                        timestamp: new Date().toISOString()
                    });
                }
                break;
                
            case 'subscribe':
                // Subscribe to device events
                this.sendToClient(clientId, {
                    type: 'subscribed',
                    devices: message.devices,
                    timestamp: new Date().toISOString()
                });
                break;
        }
    }
    
    broadcast(message) {
        const json = JSON.stringify(message);
        this.clients.forEach(client => {
            if (client.ws.readyState === WebSocket.OPEN) {
                client.ws.send(json);
            }
        });
    }
    
    sendToClient(clientId, message) {
        const client = this.clients.get(clientId);
        if (client && client.ws.readyState === WebSocket.OPEN) {
            client.ws.send(JSON.stringify(message));
        }
    }
    
    start() {
        this.server.listen(this.port, () => {
            console.log(`
╔══════════════════════════════════════════════════════════════╗
║         WebUSB WebSocket Server - RP2350-OATH               ║
╠══════════════════════════════════════════════════════════════╣
║  Server:  ws://localhost:${this.port}                        ║
║  REST:    http://localhost:${this.port}/api                 ║
║  Health:  http://localhost:${this.port}/health              ║
║                                                              ║
║  Endpoints:                                                  ║
║    POST /api/devices/register    - Register device           ║
║    GET  /api/devices             - List devices              ║
║    POST /api/devices/:id/notify  - Send notification         ║
║    POST /api/broadcast           - Broadcast to all clients  ║
║                                                              ║
║  WebSocket:                                                  ║
║    ?type=device&deviceId=ID      - Device connection         ║
║    ?type=client&clientId=ID      - Client connection         ║
╚══════════════════════════════════════════════════════════════╝
            `);
        });
    }
}

// CLI Usage
if (require.main === module) {
    const port = process.env.PORT || 8080;
    const server = new WebUSBWebSocketServer(port);
    server.start();
}

module.exports = WebUSBWebSocketServer;