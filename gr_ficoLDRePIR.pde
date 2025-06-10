import processing.serial.*;

Serial porta;
String dadoRecebido = "";

int[] ldrData = new int[200];
int[] pirData = new int[200];
int index = 0;

void setup() {
  size(800, 400);
  println(Serial.list()); // Mostra as portas disponíveis no console

  // Altere aqui para a porta correta do seu Arduino:
  porta = new Serial(this, "COM6", 9600); 
  porta.bufferUntil('\n');
}

void draw() {
  background(30);
  
  // Desenha eixo
  stroke(255);
  line(40, 50, 40, height - 50); // eixo Y
  line(40, height - 50, width - 20, height - 50); // eixo X

  // Títulos
  fill(255);
  text("Gráfico em Tempo Real: LDR (azul) & PIR (vermelho)", 250, 20);

  // Desenha gráfico do LDR
  stroke(0, 0, 255);
  noFill();
  beginShape();
  for (int i = 0; i < ldrData.length - 1; i++) {
    float x = map(i, 0, ldrData.length - 1, 50, width - 30);
    float y = map(ldrData[i], 0, 1023, height - 50, 60);
    vertex(x, y);
  }
  endShape();

  // Desenha gráfico do PIR
  stroke(255, 0, 0);
  noFill();
  beginShape();
  for (int i = 0; i < pirData.length - 1; i++) {
    float x = map(i, 0, pirData.length - 1, 50, width - 30);
    float y = map(pirData[i], 0, 1, height - 50, 60); // PIR: só 0 ou 1
    vertex(x, y);
  }
  endShape();
}

void serialEvent(Serial porta) {
  dadoRecebido = trim(porta.readStringUntil('\n'));

  if (dadoRecebido != null && dadoRecebido.contains(",")) {
    String[] partes = split(dadoRecebido, ',');
    
    if (partes.length == 2) {
      int ldr = int(partes[0]);
      int pir = int(partes[1]);

      ldrData = shiftLeft(ldrData);
      pirData = shiftLeft(pirData);

      ldrData[ldrData.length - 1] = ldr;
      pirData[pirData.length - 1] = pir;
    }
  }
}

// Função para deslocar os dados para a esquerda (scroll)
int[] shiftLeft(int[] array) {
  for (int i = 0; i < array.length - 1; i++) {
    array[i] = array[i + 1];
  }
  return array;
}
