#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <time.h>
#include "arducam.h"
#define OV5642_CHIPID_HIGH 0x300a
#define OV5642_CHIPID_LOW 0x300b

#define BUF_SIZE (384*1024)
uint8_t buffer[BUF_SIZE] = {0xFF};
const uint16_t TRANSFER_SIZE = 2048;

const char* filename = "test.jpg";

static struct timeval tm1;

static inline void start()
{
    gettimeofday(&tm1, NULL);
}

static inline void stop()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf("%llu ms\n", t);
}

void setup() {
  uint8_t vid, pid, temp;
  wiring_init();
  arducam(smOV5642,CAM1_CS,-1,-1,-1);

  // Check if the ArduCAM SPI bus is OK
  arducam_write_reg(ARDUCHIP_TEST1, 0x55, CAM1_CS);
  temp = arducam_read_reg(ARDUCHIP_TEST1, CAM1_CS);

  if(temp != 0x55) {
    printf("SPI interface error!\n");
    exit(EXIT_FAILURE);
  }
  else{
    printf("SPI interface OK!\n");
  }

  // Change MCU mode
  arducam_write_reg(ARDUCHIP_MODE, 0x00, CAM1_CS);

  // Check if the camera module type is OV5642
  arducam_i2c_word_read(OV5642_CHIPID_HIGH, &vid);
  arducam_i2c_word_read(OV5642_CHIPID_LOW, &pid);
  if((vid != 0x56) || (pid != 0x42)) {
    printf("Can't find OV5642 module!\n");
    exit(EXIT_FAILURE);
  } else {
    printf("OV5642 detected\n");
  }
}

size_t capture() {
  printf("Clearing FIFO\n");

  start();
  // Flush FIFO
  arducam_flush_fifo(CAM1_CS);
  // Clear the capture done flag
  arducam_clear_fifo_flag(CAM1_CS);

  // Start capture
  printf("Start capture\n");
  arducam_start_capture(CAM1_CS);

  // Wait untill the camera is done capturing
  while (!(arducam_read_reg(ARDUCHIP_TRIG, CAM1_CS) & CAP_DONE_MASK)) ;
  printf("Capture Done\n");

  printf("Reading FIFO\n");

  // Get length of image data
  size_t len = read_fifo_length(CAM1_CS);

  // Exit if there was a data overflow problem
  if (len >= 393216) {
    printf("Over size.");
    exit(EXIT_FAILURE);
  // Exit if the data couldn't be read
  } else if (len == 0 ) {
    printf("Size is 0.");
    exit(EXIT_FAILURE);
  }

  // Disable bus priority
  digitalWrite(CAM1_CS, LOW);

  // Start reading
  set_fifo_burst(BURST_FIFO_READ);

  int32_t i = 0;
  while(len > TRANSFER_SIZE) {
    arducam_transfers(&buffer[i], TRANSFER_SIZE);
    len -= TRANSFER_SIZE;
    i += TRANSFER_SIZE;
  }
  arducam_spi_transfers(&buffer[i], len);

  printf("Data transfered\n");

  // Enable bus priority
  digitalWrite(CAM1_CS, HIGH);

  stop();

  return len + i;
}

int main(int argc, char *argv[]) {
  // Run setup
  setup();
  // Set output format to JPEG
  arducam_set_format(fmtJPEG);
  // Set resolution to HD
  arducam_OV5642_set_jpeg_size(OV5642_640x480);
  // wait to let the camera perform the auto exposure correction
  sleep(1);

  // Enable VSYNC
  arducam_write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK,CAM1_CS);

  size_t img_len = capture();

  printf("Length: %d\n", img_len);

  /*
  // Open the new file
  FILE *fp1 = fopen(filename, "w+");

  if (!fp1) {
      printf("Error: could not open %s\n", argv[2]);
      exit(EXIT_FAILURE);
  }

  fwrite(buffer, img_len, 1, fp1);
  delay(100);
  fclose(fp1);
  */

  exit(EXIT_SUCCESS);
}
