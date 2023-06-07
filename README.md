# Projeto Embarcados

Desenvolvendo um controle remoto.

## Entrega 1

### Integrantes

- Victor Assis
- :trollface:

### Ideia

Controle que liga/desliga o áudio no Google Meet, captura áudio e controla e clica o cursor do computador.

### Nome

Hybrid GM

### Usuários

Times que realizam reuniões híbridas (remotas + presenciais).

### Software/Jogo 

Google Meet

### Jornada do usuário (3 pts)

Trabalho em uma empresa remote-first. Temos escritório em Bogotá e em São Paulo, porém a maior parte do time opta pelo trabalho remoto.
É comum ter reuniões em que parte do time está no escritório e outra parte remota participando através do Google Meet.

**A experiência sempre é ruim.**

O time presencial, para evitar microfonia, acessa a reunião através de um único computador e espelha a tela na televisão.
Por ter apenas um microfone, as pessoas que estão mais distantes do computador nunca são ouvidas por quem participa remotamente.
Outro problema, é que apenas o dono do computador controla o mouse. Durante apresentações via espelhamento de tela o controle do mouse é crucial.

Vou construir um controle que permite deslocar o cursos do computador e realizar "cliques no botão erquerdo do mouse".
Por fim, vai possuir um microfone embutido, permitindo quem está apresentando ser ouvido por todos os remotos.

### Comandos/ Feedbacks (2 pts)

[Referência de atalhos do Google Meet](https://support.google.com/a/users/answer/9896256?hl=en)
- Ligar/desligar o compartilhamento de audio.
- Controlar o cursor do mouse.
- Realizar cliques com o mouse.
- Capturar audio.

### In/OUT (3 pts)

- Ligar/desligar o compartilhamento de audio: Mini illuminated pushbutton.
- Controlar o cursor do mouse: Joypad.
- Realizar cliques com o mouse: Joypad (botão de pressionado).
- Capturar audio: Microfone.

### Design (2 pts)

Pensei em fazer no formato de um microfone de mão. O pushbutton já é comum em microfones, apenas adicionaria o joypad.
