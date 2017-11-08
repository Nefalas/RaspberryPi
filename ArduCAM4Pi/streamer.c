#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include "arducam.h"
#include "ov5642_regs.h"
#define OV5642_CHIPID_HIGH 0x300a
#define OV5642_CHIPID_LOW 0x300b

#define BUF_SIZE (384*1024)
uint8_t buffer[BUF_SIZE] = {0xFF};

const char* filename = "test.jpg";

void setup() {
  uint8_t vid,pid;
  uint8_t temp;
  wiring_init();
  arducam(smOV5642,CAM1_CS,-1,-1,-1);
  // Check if the ArduCAM SPI bus is OK
  arducam_write_reg(ARDUCHIP_TEST1, 0x55, CAM1_CS);
  temp = arducam_read_reg(ARDUCHIP_TEST1, CAM1_CS);
  //printf("temp=%x\n",temp);  //  debug
  if(temp != 0x55) {
    printf("SPI interface error!\n");
    //exit(EXIT_FAILURE);
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

int main(int argc, char *argv[]) {
  // Run setup
  setup();
  // Set output format to JPEG
  arducam_set_format(fmtJPEG);
  // Set resolution to HD
  arducam_OV5642_set_jpeg_size(OV5642_1280x720);
  // arducam_OV5642_set_jpeg_size(OV5642_1920x1080);
  // wait to let the camera perform the auto exposure correction
  sleep(1);

  // Enable VSYNC
  arducam_write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK,CAM1_CS);

  // Flush the FIFO
  arducam_flush_fifo(CAM1_CS);
  // Clear the capture done flag
  arducam_clear_fifo_flag(CAM1_CS);

  // Start capture
  printf("Start capture\n");
  arducam_start_capture(CAM1_CS);

  // Wait untill the camera is done capturing
  while (!(arducam_read_reg(ARDUCHIP_TRIG,CAM1_CS) & CAP_DONE_MASK)) ;
  printf("Capture Done\n");

  // Generate JPEG file for testing
  printf("Saving capture as %s\n", filename);

  // Open the new file
  FILE *fp1 = fopen(filename, "w+");

  if (!fp1) {
    printf("Error: could not open %s\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  printf("Reading FIFO\n");

  size_t len = read_fifo_length(CAM1_CS);
  if (len >= 393216){
    printf("Over size.");
    exit(EXIT_FAILURE);
  }else if (len == 0 ){
    printf("Size is 0.");
    exit(EXIT_FAILURE);
  }
  digitalWrite(CAM1_CS,LOW);  //Set CS low
  set_fifo_burst(BURST_FIFO_READ);
  arducam_spi_transfers(buffer,1);//dummy read
  int32_t i=1; // First bit needs to be 0xff
  while(len>4096)
  {
    arducam_transfers(&buffer[i],4096);
    len -= 4096;
    i += 4096;
  }
  arducam_spi_transfers(&buffer[i],len);

  fwrite(buffer, len+i, 1, fp1);
  digitalWrite(CAM1_CS,HIGH);  //Set CS HIGH
  //Close the file
  delay(100);
  fclose(fp1);
  // Clear the capture done flag
  arducam_clear_fifo_flag(CAM1_CS);

  exit(EXIT_SUCCESS);
}
