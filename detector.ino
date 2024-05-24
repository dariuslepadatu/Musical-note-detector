#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Initialize the LCD with I2C address 0x27 and size 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

int in[128]; // Array to store analog input samples
byte NoteV[13] = {8, 23, 40, 57, 76, 96, 116, 138, 162, 187, 213, 241, 255}; // Data for note detection based on frequency
float f_peaks[5]; // Array to store the top 5 frequency peaks in descending order
int Mic_pin; // Microphone pin
float last_freq = 0; // Variable to store the last detected frequency
const int buzzer = 3; // Buzzer pin
const int button_pin = 2;
 
struct button {
  byte pressed = 0;
};
button button;
 

void setup() {
  Serial.begin(250000); // Initialize serial communication at 250000 baud
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the LCD backlight
  pinMode(button_pin, INPUT_PULLUP); // initialize the button

  Mic_pin = A0; // Set the microphone pin to A0
  pinMode(buzzer, OUTPUT); // Set the buzzer pin as output
}

// Function to print the detected note on the LCD
void printOnLCD(const char* note) {
  lcd.clear(); // Clear the LCD screen
  lcd.setCursor(3, 0); // Set the cursor to the fourth column of the first row
  lcd.print(note); // Print the note on the LCD
}
 
bool isButtonPressed(int pin)
{
  return digitalRead(pin) == 0;
}


const char *last = "";
bool isPressed = false;
void loop() {
   button.pressed = isButtonPressed(button_pin);
 
  if (button.pressed) {
    isPressed = !isPressed;
  }
  const char *out = Tone_det(); // Detect the current tone

  if (out != last) {
     printOnLCD(out); // Print the detected tone on the LCD
  }
  if (isPressed and out != "UNKNOWN") {
      tone(buzzer, last_freq); // Play the detected frequency on the buzzer
      delay(1000); // Wait for 1 second
      noTone(buzzer); // Stop the buzzer
  }
  last = out;
  delay(100); // Wait for 100 milliseconds 
}

// Function to detect the tone
const char* Tone_det() {
  long unsigned int a1, b;
  float a;
  float sum1 = 0, sum2 = 0;
  float sampling;
  a1 = micros(); // Get the current time in microseconds
  for (int i = 0; i < 128; i++) {
    a = analogRead(Mic_pin) - 500; // Read the analog value from the microphone and zero-shift it
    sum1 = sum1 + a; // Sum for average amplitude calculation
    sum2 = sum2 + a * a; // Sum for RMS amplitude calculation
    a = a * (sin(i * 3.14 / 128) * sin(i * 3.14 / 128)); // Apply Hann window function
    in[i] = 10 * a; // Scale the value for float to int conversion
    delayMicroseconds(195); // Delay to match the sampling rate
  }
  b = micros(); // Get the current time in microseconds

  sum1 = sum1 / 128; // Calculate average amplitude
  sum2 = sqrt(sum2 / 128); // Calculate RMS amplitude
  sampling = 128000000 / (b - a1); // Calculate the real-time sampling frequency

  // Only proceed if the amplitude is significant
  if (sum2 - sum1 > 3) {
    FFT(128, sampling); // Perform FFT to find the frequency peaks

    for (int i = 0; i < 12; i++) {
      in[i] = 0; // Reset the in[] array for further calculation
    }

    // Convert frequency peaks to musical notes
    int j = 0, k = 0;
    for (int i = 0; i < 5; i++) {
      if (f_peaks[i] > 1040) {
        f_peaks[i] = 0;
      }
      if (f_peaks[i] >= 65.4 && f_peaks[i] <= 130.8) {
        f_peaks[i] = 255 * ((f_peaks[i] / 65.4) - 1);
      }
      if (f_peaks[i] >= 130.8 && f_peaks[i] <= 261.6) {
        f_peaks[i] = 255 * ((f_peaks[i] / 130.8) - 1);
      }
      if (f_peaks[i] >= 261.6 && f_peaks[i] <= 523.25) {
        f_peaks[i] = 255 * ((f_peaks[i] / 261.6) - 1);
      }
      if (f_peaks[i] >= 523.25 && f_peaks[i] <= 1046) {
        f_peaks[i] = 255 * ((f_peaks[i] / 523.25) - 1);
      }
      if (f_peaks[i] >= 1046 && f_peaks[i] <= 2093) {
        f_peaks[i] = 255 * ((f_peaks[i] / 1046) - 1);
      }
      if (f_peaks[i] > 255) {
        f_peaks[i] = 254;
      }
      j = 1;
      k = 0;

      // Find the corresponding note for each frequency peak
      while (j == 1) {
        if (f_peaks[i] < NoteV[k]) {
          f_peaks[i] = k;
          j = 0;
        }
        k++;
        if (k > 15) {
          j = 0;
        }
      }

      if (f_peaks[i] == 12) {
        f_peaks[i] = 0;
      }
      k = f_peaks[i];
      in[k] = in[k] + (5 - i);
    }

    // Detect the note with the maximum value
    k = 0;
    j = 0;
    for (int i = 0; i < 12; i++) {
      if (k < in[i]) {
        k = in[i];
        j = i;
      }
    }

    // Return the detected note as a string
    k = j;
    if (k == 0) return "C";
    if (k == 1) return "C#";
    if (k == 2) return "D";
    if (k == 3) return "D#";
    if (k == 4) return "E";
    if (k == 5) return "F";
    if (k == 6) return "F#";
    if (k == 7) return "G";
    if (k == 8) return "G#";
    if (k == 9) return "A";
    if (k == 10) return "A#";
    if (k == 11) return "B";
  }
  last_freq = f_peaks[0]; // Update the last detected frequency
  return "UNKNOWN"; // Return "UNKNOWN" if no note is detected
}

// Function to perform Fast Fourier Transform (FFT)
float FFT(byte N, float Frequency) {
  byte data[8] = {1, 2, 4, 8, 16, 32, 64, 128};
  int a, c1, f, o, x;
  a = N;

  // Calculate the levels for FFT
  for (int i = 0; i < 8; i++) {
    if (data[i] <= a) {
      o = i;
    }
  }
  o = 7;
  byte in_ps[data[o]] = {}; // Input for sequencing
  float out_r[data[o]] = {}; // Real part of transform
  float out_im[data[o]] = {}; // Imaginary part of transform

  x = 0;
  // Bit reversal process
  for (int b = 0; b < o; b++) {
    c1 = data[b];
    f = data[o] / (c1 + c1);
    for (int j = 0; j < c1; j++) {
      x = x + 1;
      in_ps[x] = in_ps[j] + f;
    }
  }

  // Update input array as per bit reverse order
  for (int i = 0; i < data[o]; i++) {
    if (in_ps[i] < a) {
      out_r[i] = in[in_ps[i]];
    }
    if (in_ps[i] > a) {
      out_r[i] = in[in_ps[i] - a];
    }
  }

  int i10, i11, n1;
  float e, c, s, tr, ti;

  // Perform FFT
  for (int i = 0; i < o; i++) {
    i10 = data[i]; // Overall values of sine and cosine
    i11 = data[o] / data[i + 1]; // Loop with similar sine and cosine values
    e = 6.283 / data[i + 1];
    e = 0 - e;
    n1 = 0;

    for (int j = 0; j < i10; j++) {
      c = cos(e * j);
      s = sin(e * j);
      n1 = j;

      for (int k = 0; k < i11; k++) {
        tr = c * out_r[i10 + n1] - s * out_im[i10 + n1];
        ti = s * out_r[i10 + n1] + c * out_im[i10 + n1];

        out_r[n1 + i10] = out_r[n1] - tr;
        out_r[n1] = out_r[n1] + tr;

        out_im[n1 + i10] = out_im[n1] - ti;
        out_im[n1] = out_im[n1] + ti;

        n1 = n1 + i10 + i10;
      }
    }
  }

  // Calculate amplitude from complex numbers
  for (int i = 0; i < data[o - 1]; i++) {
    out_r[i] = sqrt((out_r[i] * out_r[i]) + (out_im[i] * out_im[i])); // To increase the speed, delete sqrt
    out_im[i] = (i * Frequency) / data[o];
  }

  // Peak detection
  x = 0;
  for (int i = 1; i < data[o - 1] - 1; i++) {
    if (out_r[i] > out_r[i - 1] && out_r[i] > out_r[i + 1]) {
      in_ps[x] = i; // Store the peak number
      x = x + 1;
    }
  }

  // Re-arrange as per magnitude
  s = 0;
  c = 0;
  for (int i = 0; i < x; i++) {
    for (int j = c; j < x; j++) {
      if (out_r[in_ps[i]] < out_r[in_ps[j]]) {
        s = in_ps[i];
        in_ps[i] = in_ps[j];
        in_ps[j] = s;
      }
    }
    c = c + 1;
  }

  // Update f_peak array (global variable) with descending order of peaks
  for (int i = 0; i < 5; i++) {
    f_peaks[i] = (out_im[in_ps[i] - 1] * out_r[in_ps[i] - 1] +
                  out_im[in_ps[i]] * out_r[in_ps[i]] +
                  out_im[in_ps[i] + 1] * out_r[in_ps[i] + 1]) /
                 (out_r[in_ps[i] - 1] + out_r[in_ps[i]] + out_r[in_ps[i] + 1]);
  }
}
