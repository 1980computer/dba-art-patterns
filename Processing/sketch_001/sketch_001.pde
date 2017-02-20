static final int numberLines = 30;
 float t;
 
 void setup(){
   background(10);
   size(1024,768);
 }
 
 void draw(){
   background(10);
 stroke(255);
 strokeWeight(1);
 translate(width/2, height/2);
 
 
 for( int i = 0; i < numberLines; i++){
 line( x1(t +i), y1(t+i), x2(t+i), y2(t+i));
 }
 t+=0.5;
 }
 
 float x1(float t){
 return sin(t/10) *200 + sin(t/5) *20;
 }
 
  float y1 (float t){
 return cos(t/10)* 200;
 }
 
  float x2(float t){
 return sin(t/10) *800 + sin(t) *2;
 }
 
  float y2 (float t){
 return cos(t/100)* 1000 + cos (t / 12) *20;
 }