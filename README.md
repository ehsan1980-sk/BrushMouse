# BrushMouse
The project for robo-competition in the Shimoyama Lab.
With websocket and tcp socket communication, you can control the mini-car (called "BrushMouse") with your smart phone.
Use the optical censor extracted from the optical mouse on the market.
The optical censor output compensate the movement of the "BrushMouse".

## structure
- Use ESP-Wroom02 as controller for the car.
- Nodejs works in the server side.
- The web page in the client communicates with Nodejs server (websocket).
- Nodejs server communicates with ESP-Wroom02 (tcp socket).
- PWM output for motors are controlled by the smart phone and the optical censor feedback.

## Client 
Acess [server's IP adress]:8000

## BrushMouse
- Optical censor is PAW3205DB ( http://www.pixart.com.tw/upload/PAW3205DB-TJ3T_DS_S_V1.0_20130514113310.pdf )

