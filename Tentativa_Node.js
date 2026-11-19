Instale o Node.js: Baixe e instale a versão LTS no PC dele (igual você fez no seu).

Pareie o Bluetooth: Conecte o PC dele ao módulo Bluetooth do Arduino.

Descubra a Porta COM: Aperte a tecla Windows, digite Gerenciador de Dispositivos, vá em "Portas (COM e LPT)" e veja qual número COM o Windows deu para o Bluetooth.

Prepare a Pasta: Cole a pasta do seu projeto no PC dele. Abra o terminal dentro dessa pasta e rode o comando de instalação para baixar as bibliotecas no PC dele:

Bash
npm init -y
npm install express socket.io serialport @serialport/parser-readline
Ajuste a Porta: Abra o arquivo server.js, mude COM3 para a porta que você descobriu no passo 3 e salve. *****************************************************

Rode o Servidor: No terminal, digite node server.js.

Acesse: No PC do amigo, abra http://localhost:3000. No tablet (conectado no mesmo Wi-Fi), digite o IP do PC do amigo seguido de :3000 (ex: [http://192.168.1.15:3000](http://192.168.1.15:3000)).

Os Arquivos (Para Copiar e Colar)
Como o design (cores, botões) e a lógica de animação do elevador são seus, eu preparei exatamente o que você precisa substituir e adicionar em cada arquivo para não perder o seu trabalho visual.


  1. Crie o arquivo server.js (Novo arquivo na pasta)
Copie e cole este código inteiro em um arquivo novo chamado server.js:

JavaScript
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Hospeda os seus arquivos HTML, CSS e JS na rede
app.use(express.static(__dirname));

// ==========================================
// ⚠️ ALTERE A PORTA COM AQUI NO DIA DO TESTE
// ==========================================
const portaSerial = new SerialPort({ path: 'COM3', baudRate: 9600 });
const parser = portaSerial.pipe(new ReadlineParser({ delimiter: '\n' }));

// Recebe dados do Arduino e espalha para PC e Tablet
parser.on('data', (dado) => {
    io.emit('dados_arduino', dado);
});

// Recebe comandos do Tablet/PC e manda para o Arduino
io.on('connection', (socket) => {
    console.log('Novo dispositivo conectado à IHM Web!');
    
    socket.on('comando_web', (cmd) => {
        portaSerial.write(cmd);
    });
});

server.listen(3000, '0.0.0.0', () => {
    console.log('✅ Servidor SCADA Online!');
    console.log('-> Acesse no PC: http://localhost:3000');
});
2. Adicione no index.html (Não apague o seu código)
Vá no seu arquivo HTML e cole apenas estas duas linhas nos lugares indicados:

Dentro da tag <head> (perto do título):

HTML
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
No final do arquivo (logo acima da linha <script src="script.js">):

HTML
<script src="/socket.io/socket.io.js"></script>
3. Adicione no styles.css (Não apague o seu código)
Vá até a última linha do seu arquivo CSS e cole este bloco. Isso vai enquadrar o supervisório no tablet:

CSS
/* =========================================
   RESPONSIVIDADE PARA TABLETS E CELULARES
   ========================================= */
@media (max-width: 992px) {
    body { 
        padding: 10px; 
        margin: 0; 
    }
    
    /* Faz os painéis ficarem um embaixo do outro */
    .container {
        flex-direction: column !important;
        width: 100% !important;
    }
    
    .painel {
        width: 100% !important;
        margin-bottom: 20px;
    }
    
    /* Aumenta a área de toque dos botões */
    button, .botao {
        width: 100% !important;
        height: 65px;
        font-size: 1.3rem;
        margin-bottom: 12px;
        touch-action: manipulation;
    }
}
4. Substitua no script.js (Onde a mágica da rede acontece)
Apague apenas a parte de cima do seu arquivo JS (as variáveis port, reader, writer e as funções de conectar e ler a porta Serial).

No lugar da parte apagada, cole este código abaixo. Mantenha intactas as suas funções do final do arquivo (como a processarTagEspecial ou funções de som e log, pois elas continuam controlando a tela perfeitamente).

JavaScript
// Conecta a tela ao Servidor Node.js via Wi-Fi
const socket = io();

// Pega os elementos da tela (ajuste os IDs se os seus forem diferentes)
const btnConectar = document.getElementById('btnConectar');
const statusTxt = document.getElementById('statusTxt');

// O botão conectar agora serve apenas para dar um feedback visual na tela
btnConectar.addEventListener('click', () => {
    statusTxt.innerText = "Status: Conectado na Rede SCADA";
    statusTxt.style.color = "#00adb5";
    console.log("Sistema IHM pronto.");
    // tocarSomClique(); // Descomente se usar a sua função de som
});

// Substitua sua antiga função enviarComando por esta:
function enviarComando(caractere) {
    socket.emit('comando_web', caractere);
    // tocarSomClique();
}

// Substitua sua antiga função lerDadosDaSerial por esta:
socket.on('dados_arduino', (linha) => {
    let linhaLimpa = linha.trim();
    if (!linhaLimpa) return;

    if (linhaLimpa.startsWith('*')) {
        // Envia para a sua função que já existe e que move o elevador na tela
        processarTagEspecial(linhaLimpa); 
    } else {
        console.log("Recebido:", linhaLimpa);
        // adicionarAoLog(linhaLimpa); // Descomente se você usa a caixa de log de texto
    }
});






Transmissao do sinal:
baixar MyPublicWiFi ou Connectify Hotspot.

  ou no cmd adm rodar: netsh wlan set hostednetwork mode=allow ssid=TCC_Elevador key=12345678 
  e depois: netsh wlan start hostednetwork

ou rotear pelo celular mesmo
