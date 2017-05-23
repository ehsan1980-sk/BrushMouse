/*
 * ESP8266
 * pin assign
 *   sw: IO16
 *   motorL: IO4
 *   motorR: IO5
 *   
 *   起動時にsw長押しで接続先設定mode, 通常はeepromのデータで自動接続する
 */
 
 /* 
  *  TODO 
  *  Straight 評価
  */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

const int swPin = 16;
const int motorLPin = 4;
const int motorRPin = 5;
const int LEDPin = 14; 
const int OptiDataPin = 12;
const int OptiClkPin = 13;

struct CONFIG {
  char ssid[20];
  char pass[20];
  char host[16];
  int port;
};

const long FCY = 80000000L;

/* 
 *  バイナリ送信して(unsigned char) にcastすること
 *  null(0)で終わりを示す
 *  command は1byteとする
 *  
 * [ControlotorCmd]  
 *   00[right][left]
 *   right : 1byte
 *   left : 1byte
 *   
 *   [ControlLEDCmd]
 *   01[led]
 *   led : 1byte
 *   
 *   [OpticalCmd]
 *   20[spX][spY][X][Y][deltaSpeedY]
 *   spX: 2byte
 *   spY: 2byte
 *   X: 4byte
 *   Y: 4byte
 *   deltaSpeedY : 2byte
 */
const unsigned char ControlMotorCmd = 0x10;
const unsigned char ControlLEDCmd = 0x12;
const unsigned char OpticalCmd = 0x20;

const unsigned char LedOnStatus = 0x11;

int correctPId1 = 0x31;

int cnt1 = 0;
WiFiClient client;

/*
 *  OpticalCensorまわり
 * 
 */
const uint8_t OptiRegDx = 0x03; // 0x03
const uint8_t OptiRegDy = 0x04; // 0x04
const uint8_t OptiRegMotion = 0x02;
const uint8_t OptiRegProd1 =0x00;

int32_t optiX;
int32_t optiY;
signed char optiProd1;
signed char optiMotion;

 int16_t optiSpX;
 int16_t optiSpY;

 int16_t deltaSpeedY;

 const unsigned int optiSpCof = 800; // 1000分率
 const int optiSpDif = 1000;

uint8_t OptiReadRegister(uint8_t address)
{
  int i = 7;
  uint8_t r = 0;
  
  // Write the address of the register we want to read:
  pinMode (OptiDataPin, OUTPUT);
  for (i=7; i>=0; i--)
  {
    digitalWrite (OptiClkPin, LOW);
    //delayMicroseconds(50);
    digitalWrite (OptiDataPin, address & (1 << i));
    delayMicroseconds(1);
    digitalWrite (OptiClkPin, HIGH);
    delayMicroseconds(5);
  }
  
  // Switch data line from OUTPUT to INPUT
  pinMode (OptiDataPin, INPUT);
  
  // Wait a bit...
  delayMicroseconds(30);
  
  // Fetch the data!
  for (i=7; i>=0; i--)
  {                             
    digitalWrite (OptiClkPin, LOW);
    delayMicroseconds(1);
    digitalWrite (OptiClkPin, HIGH);
    delayMicroseconds(4);
    r |= (digitalRead (OptiDataPin) << i);
    delayMicroseconds(1);
  }
  delayMicroseconds(25); // いらないかも
  
  return r;
}

void OptiWriteRegister(uint8_t address, uint8_t data)
{
  int i = 7;
  
  // Set MSB high, to indicate write operation:
  address |= 0x80;
  
  // Write the address:
  pinMode (OptiDataPin, OUTPUT);
  for (; i>=0; i--)
  {
    digitalWrite (OptiClkPin, LOW);
    //delayMicroseconds(5);
    digitalWrite (OptiDataPin, address & (1 << i));
    delayMicroseconds(1);
    digitalWrite (OptiClkPin, HIGH);
    delayMicroseconds(5);
  }

  delayMicroseconds(30);
  
  // Write the data:
  for (i=7; i>=0; i--)
  {
    digitalWrite (OptiClkPin, LOW);
    //delayMicroseconds(5);
    digitalWrite (OptiDataPin, data & (1 << i));
    delayMicroseconds(1);
    digitalWrite (OptiClkPin, HIGH);
    delayMicroseconds(5);
  }
}

signed char OptiDx(){
  return  (signed char) OptiReadRegister(OptiRegDx);
}

signed char OptiDy(){
  return  (signed char) OptiReadRegister(OptiRegDy);
}

signed char OptiProductId1(){
  return  (signed char) OptiReadRegister(OptiRegProd1);
}

signed char OptiIsMotion(){
  signed char d = OptiReadRegister(OptiRegMotion);
  optiMotion = d;
  //return d & 0x80 ? 1 : 0;
  return optiMotion == -122 ? 1 : 0;
}

void OptiBegin(void)
{
  optiX = 0;
  optiY = 0;
  optiSpX = 0;
  optiSpY = 0;
  
  digitalWrite(OptiClkPin, HIGH);                     
  delayMicroseconds(5);
  digitalWrite(OptiClkPin, LOW);
  delayMicroseconds(1);
  digitalWrite(OptiClkPin, HIGH); 
  delay(100);
  
}

void OptiSetup(){
  int cnt = 0, PId1;
  pinMode (OptiClkPin, OUTPUT);
  pinMode (OptiDataPin, INPUT);
 Serial.println("OptiSetup");
 while(1){
  OptiBegin();
  PId1 = OptiProductId1();
  if( PId1 == correctPId1){
    break;
  }
  cnt++;
  if(cnt % 10 == 0){
    //cnt = 0;
    Serial.print("Id = : ");
    Serial.println(PId1 ,DEC);
  }
  if(cnt == 100)
    break;
  delay(1);
 }

 
 Serial.println("\n break while!! \n");
 Serial.println(cnt, DEC);
  
  
  OptiWriteRegister(0x89, 0xA5);
  OptiWriteRegister(0x89, 0x00);
  OptiWriteRegister(0x86, 0x00);
  OptiWriteRegister(0x86, 0x04);
  OptiWriteRegister(0x85, 0xBC);
  delay(5);
  OptiWriteRegister(0x86, 0x08);
  delay(550);
  OptiWriteRegister(0x86, 0x80);
  delay(8);
  OptiWriteRegister(0x85, 0xB9);
  OptiWriteRegister(0x86, 0x80);
  OptiWriteRegister(0x86, 0x06);
  delay(32);
    
}

void OptiReconnect(){
  int PId1, cnt = 0;
  updateLED(1);
  while(1){
    OptiBegin();
    PId1 = OptiProductId1();
    if( PId1 == correctPId1){
      break;
    }
    cnt++;
    if(cnt % 10 == 0){
     //cnt = 0;
      Serial.print("Id = : ");
      Serial.println(PId1 ,DEC);
    }
    if(cnt == 100)
      break;
    delay(1);
  }
  updateLED(0);  
}

void OptiUpdate(){
  signed char dx, dy;
  optiProd1 = OptiProductId1();
  
 if(optiProd1 == correctPId1 && OptiIsMotion() == 1)
 {
    dx = OptiDx();
    dy = OptiDy();
    // 積算
    optiX += dx;
    optiY += dy;
  }else{
    dx = 0;
    dy = 0;
    if(optiProd1 != correctPId1 ){
      optiProd1 = OptiProductId1();
      if(optiProd1 != correctPId1)
      {
        Serial.print("reconnect id1 = ");
        Serial.print(optiProd1, DEC);
        OptiReconnect();
      }
    }
  }
  // 平滑化速度算出  TODO サイクル間隔を考慮
  /*
  optiSpX *= optiSpCof;
  optiSpX += (optiSpDif - optiSpCof) * dx;
  optiSpX /= optiSpDif;

  optiSpY *= optiSpCof;
  optiSpY += (optiSpDif - optiSpCof) * dy;
  optiSpY /= optiSpDif;
    */

  optiSpX = dx;
  optiSpY = dy;
}


/* OpticalSensor まわり終了 */

// accessポイントとの接続
void wifiStart(struct CONFIG* conf){
  Serial.println("WiFi module setting... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(conf->ssid , conf->pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

// tcpサーバとの接続
void tcpStart(struct CONFIG* conf){
  while (!client.connect(conf->host, conf->port)) {//TCPサーバへの接続要求
    Serial.print(".");
  }
}

// 入力は0~1023の範囲!!
void updateMotors(int right, int left){
  int maxVal = 1023;
  int lowerLimit = 400;
  if(right > maxVal)
    right = maxVal;
  else if(right < lowerLimit)
    right = 0; 
    
  if(left > maxVal)
    left = maxVal;
  else if(left < lowerLimit)
    left = 0;
    
  analogWrite(motorRPin, right);
  analogWrite(motorLPin, left);
}

/*
 * optiセンサ値をフィードバックとし、真っすぐ進む
 * TODO パラメータ調整
 */
void updateMotorsStraight(int forward){  
  int maxVal = 1023;
  int lowerLimit = 0;//測定のため0から
  int feedP = 20;
  int deltaSpeed;
  
  const int maxSpeed = 40; // 要調整
  
    /*
  int right =  forward + optiSpX * feedP /100;
  int left = forward - optiSpX * feedP / 100;
  */
  int requiredSpeed = forward * maxSpeed / maxVal;
  deltaSpeed = requiredSpeed - optiSpX;
  forward += deltaSpeed * feedP;

  if(forward > maxVal)
    forward = maxVal;
  else if(maxVal < lowerLimit)
    forward = 0;
  
 // updateMotors(right, left); 
   updateMotors(forward, forward); 
   deltaSpeedY = (int16_t) deltaSpeed;
}

void updateLED(int stat){
  if(stat == 1)
    digitalWrite(LEDPin, HIGH);
  else
    digitalWrite(LEDPin, LOW);
}

void sendMsg(String line){
  client.print(line);//データを送信
}

void sendLines(uint8_t* line, char len){
  //while(!client.available()){}
  client.write((const uint8_t *)line, len);//データを送信
}

void sendOpticalData(){
  uint8_t writeLine[15];
  writeLine[0] = ( uint8_t)OpticalCmd;
  writeLine[1] = ( uint8_t)((((unsigned int16_t) optiSpX) & 0xff00) >> 8);
  writeLine[2] = ( uint8_t)(((unsigned int16_t) optiSpX) & 0x00ff);
  writeLine[3] = ( uint8_t)((((unsigned int16_t) optiSpY) & 0xff00) >> 8);
  writeLine[4] = ( uint8_t)(((unsigned int16_t) optiSpY) & 0x00ff);

  writeLine[5] = ( uint8_t)((((unsigned int32_t) optiX) & 0xff000000) >> 24);
  writeLine[6] = ( uint8_t)((((unsigned int32_t) optiX) & 0x00ff0000) >> 16);
  writeLine[7] = ( uint8_t)((((unsigned int32_t) optiX) & 0x0000ff00) >> 8);
  writeLine[8] = ( uint8_t)(((unsigned int32_t) optiX) & 0x000000ff);

  writeLine[9] = ( uint8_t)((((unsigned int32_t) optiY) & 0xff000000) >> 24);
  writeLine[10] = ( uint8_t)((((unsigned int32_t) optiY) & 0x00ff0000) >> 16);
  writeLine[11] = ( uint8_t)((((unsigned int32_t) optiY) & 0x0000ff00) >> 8);
  writeLine[12] = ( uint8_t)(((unsigned int32_t) optiY) & 0x000000ff);

  writeLine[13] = ( uint8_t) ((deltaSpeedY &0xff00) >> 8);
  writeLine[14] = ( uint8_t) (deltaSpeedY & 0x00ff); 
  sendLines(writeLine, 15);
}

int getRequest(){
  const unsigned char maxLine = 10;
  unsigned char readLine[maxLine] ;
  int index = 0, right, left;
  char cmd;

  /*   
   * 読み込み
   */
  while(!client.available()){}
  while(client.available() && index < maxLine)
  {
    readLine[index] = (unsigned char)client.read();
    
    if(readLine[index] == 0) // null終端
      break;
    Serial.print(readLine[index], DEC); // for debug
    Serial.print(":");
    index++;
  }
  Serial.println("");

  /*   
   * コマンドparse
   */
  if(index == maxLine || index < 1)
    return -1;
 
  cmd = readLine[0];
  switch(cmd){
    case ControlMotorCmd:
      if(index < 3)
        return -1;
      right = (int) (readLine[1] << 2);
      left = (int) (readLine[2] << 2);
      if(right == left)
        updateMotorsStraight(right);
      else
        updateMotors(right, left);
      break;

     case ControlLEDCmd:
      if(index < 2)
        return -1;
      updateLED(readLine[1] ==  LedOnStatus ? 1 : 0);
      break;

     default:
      return 0;
  }
  
  return 1;
}

void timer0_ISR (void) {
   Serial.print("id1=");
   Serial.print(optiProd1, DEC);
   Serial.print(" motion=");
   Serial.print(optiMotion, DEC);
   Serial.print(" x=");
   Serial.print(optiX, DEC);
   Serial.print(" y=");
   Serial.print(optiY, DEC);
   Serial.println(); // for \n
   timer0_write(ESP.getCycleCount() + FCY/10); 
}

void Timer0Setup(){
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timer0_ISR);
  timer0_write(ESP.getCycleCount() + FCY); // 80MHz == 1sec
  interrupts();
}

int getHostname(struct CONFIG* conf){ // 成功したら１、　失敗-1
  char c;
  int i;
  int success = 100;
   // つなげる相手先のhost名をUARTで取得、次回以降はEEPROM からよみだせるようにする
  Serial.println("please tell me the host.");
  Serial.println("example : \"p192.168.10.17p\" ");
  i = 0;
  while(digitalRead(swPin) == 0){ // sw おされたらぬける
    if(Serial.available()){
      c = Serial.read();
      if(c == 'p'){
        if(i==0){
          i=1;
        }else{
          i = success;// fin
          conf->host[i-1] = 0; //null終端
          break;
        }
      }else{
        if(i > 0){ // read
          conf->host[i-1] = c;
          i++;
        }
      }
    }
  }
  
 if(i!=success){
  return -1;
 }
 return 1;
}

// eepromからconfデータ読出し
void readEepromConf(struct CONFIG* conf){
  EEPROM.begin(100); // 使用するサイズを宣言する
  EEPROM.get<CONFIG>(0, *conf);
  Serial.println("\nRead from eeprom");
  Serial.println(conf->ssid);
  Serial.println(conf->pass);
  Serial.println(conf->host);
  Serial.println(String(conf->port));
}

void writeEepromConf(struct CONFIG* conf){
   EEPROM.begin(100); // 使用するサイズを宣言する
   Serial.print("[writeEeprom] Your setting host is");
   Serial.println(conf->host);
   EEPROM.put<CONFIG>(0, *conf);
   EEPROM.commit();
}

//初期化処理
void setup() {
  signed int i;
  //ESP.wdtEnable(15000);
  // ESP.wdtDisable();
  Serial.begin(115200);
  delay(10);

  pinMode(swPin, INPUT);
  pinMode(motorLPin, OUTPUT);
  pinMode(motorRPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);

  pinMode (OptiClkPin, OUTPUT);
  pinMode (OptiDataPin, INPUT);

  digitalWrite(LEDPin, HIGH);
  
  noInterrupts();

  struct CONFIG conf = {
    "wx02-e4f52a",
    "7f8f37091d9ae",
    "192.168.179.3",
    11111
  };
  
  i=0;
  if(digitalRead(swPin) == 1){ //sw おされていたら設定
    delay(300);
    while(digitalRead(swPin)){}
    delay(300);
    i = getHostname(&conf);
  }

  if(i==1)
    writeEepromConf(&conf);
  else if(i == -1)
    writeEepromConf(&conf); // コードのを使用
  else
    readEepromConf(&conf);
  
  
  wifiStart(&conf);
  tcpStart(&conf);
  
  Serial.println("");
  Serial.println("Start");
  digitalWrite(LEDPin, LOW);

  OptiSetup();
  
  //Timer0Setup();
  
}

//本体処理
void loop() {  // FIXME wdtが作動しないように
  
  //  センサinput(真っすぐモード以外の活用法は？)
   OptiUpdate();
   //gupdateMotorsStraight();
   
   if(client.available())
    getRequest();

   delay(8);
   cnt1++;
   if(cnt1 > 30){
    cnt1 = 0;
    Serial.print("id1=");
    Serial.print(optiProd1, DEC);
    Serial.print(" motion=");
    Serial.print(optiMotion, DEC);
    Serial.print(" x=");
    Serial.print(optiX, DEC);
    Serial.print(" y=");
    Serial.print(optiY, DEC);
    
    Serial.print(" spx=");
    Serial.print(optiSpX, DEC);
    Serial.print(" spy=");
    Serial.print(optiSpY, DEC);
    
    Serial.println(); // for \n
    sendOpticalData();
   
   }
}
