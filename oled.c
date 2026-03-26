#include <efm8lb1.h>
#include <stdint.h>
#define SCL P0_0 
#define SDA P0_1 

// OLED I2C Address (Usually 0x78 or 0x7A for SSD1306)
#define OLED_ADDR 0x78

// --- Hardware Pins for I2C ---
// Change these if you plugged your OLED into different pins!
 

// OLED I2C Address (Usually 0x78 or 0x7A for SSD1306)
#define OLED_ADDR 0x78

// --- Global Variables ---
unsigned char xdata page_buffer[128];


// --- I2C Bit-Banging Functions ---
void I2C_Delay(void) {
    unsigned char i;
    for(i = 0; i < 5; i++); // Tiny delay to stabilize signals
}

void I2C_Start(void) {
    SDA = 1; SCL = 1; I2C_Delay();
    SDA = 0; I2C_Delay();
    SCL = 0;
}

void I2C_Stop(void) {
    SDA = 0; SCL = 0; I2C_Delay();
    SCL = 1; I2C_Delay();
    SDA = 1; I2C_Delay();
}

void I2C_Write(unsigned char dat) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        SDA = (dat & 0x80) ? 1 : 0;
        I2C_Delay();
        SCL = 1; I2C_Delay();
        SCL = 0;
        dat <<= 1;
    }
    // Clock the ACK bit from the OLED (we just ignore reading it)
    SDA = 1; I2C_Delay();
    SCL = 1; I2C_Delay();
    SCL = 0;
}

// --- OLED Communication Functions ---
void OLED_Command(unsigned char cmd) {
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x00); // 0x00 means "I am sending a command"
    I2C_Write(cmd);
    I2C_Stop();
}

void OLED_Write_Buffer(unsigned char *buf, unsigned char len) {
    unsigned char i;
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x40); // 0x40 means "I am sending data"
    for (i = 0; i < len; i++) {
        I2C_Write(buf[i]);
    }
    I2C_Stop();
}

// --- Drawing Logic ---
void ClearBuffer(void) {
    unsigned char i;
    for (i = 0; i < 128; i++) {
        page_buffer[i] = 0;
    }
}

void DrawRectToBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t current_page) {
    uint8_t x, y;
    uint8_t column_data;
    uint8_t page_top = current_page * 8;
    uint8_t page_bottom = page_top + 7;

    if (y1 > page_bottom || y2 < page_top) return;

    for (x = x1; x <= x2; x++) {
        column_data = 0;

        if (x == x1 || x == x2) {
            for (y = y1; y <= y2; y++) {
                if (y >= page_top && y <= page_bottom) column_data |= (1 << (y - page_top));
            }
        }
        
        if (y1 >= page_top && y1 <= page_bottom) column_data |= (1 << (y1 - page_top));
        if (y2 >= page_top && y2 <= page_bottom) column_data |= (1 << (y2 - page_top));

        page_buffer[x] |= column_data;
    }
}

void DrawFilledRectToBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t current_page) {
    uint8_t x, y;
    uint8_t column_data;
    uint8_t page_top = current_page * 8;
    uint8_t page_bottom = page_top + 7;

    // Check if the rectangle exists on this horizontal page
    if (y1 > page_bottom || y2 < page_top) return;

    // Loop through every column from the left edge to the right edge
    for (x = x1; x <= x2; x++) {
        column_data = 0;

        // Loop through every Y pixel from top to bottom of the rectangle
        for (y = y1; y <= y2; y++) {
            // If the pixel falls within the current 8-pixel page, set the bit
            if (y >= page_top && y <= page_bottom) {
                column_data |= (1 << (y - page_top));
            }
        }

        // Apply the solid column data to the buffer
        page_buffer[x] |= column_data;
    }
}

// --- Initialization ---
void Init_MCU(void) {
    SFRPAGE = 0x00;
    WDTCN = 0xDE; // Disable watchdog timer
    WDTCN = 0xAD;

    CLKSEL = 0x00;  // Use internal oscillator
    P0MDOUT = 0x00; // All Port 0 pins open-drain (required for I2C)
    XBR2 = 0x40;    // Enable Crossbar
}

void OLED_Init(void) {
    OLED_Command(0xAE); // Display OFF
    OLED_Command(0x20); // Set Memory Addressing Mode
    OLED_Command(0x02); // Set Page Addressing Mode
    OLED_Command(0x8D); // Charge pump setup
    OLED_Command(0x14); // Enable charge pump
    OLED_Command(0xAF); // Display ON
}

// --- Main Program ---


void main(void) {
    uint8_t p;

    Init_MCU();
    OLED_Init();

    while (1) {
        // Render the screen page by page (8 pages total)
        for (p = 0; p < 8; p++) {
            ClearBuffer();
            
            
            // P
            // Draw a rectangle from (X:10, Y:10) to (X:50, Y:30)
            DrawFilledRectToBuffer(10, 10, 13, 35, p);
            DrawFilledRectToBuffer(13, 10, 18, 15, p);
            DrawFilledRectToBuffer(13, 20, 18, 25, p);
            DrawFilledRectToBuffer(18, 10, 20, 25, p);
            
            // A
            DrawFilledRectToBuffer(30, 10, 35, 35, p);
          //  DrawFilledRectToBuffer(13, 10, 18, 15, p);
          //  DrawFilledRectToBuffer(13, 20, 18, 25, p);
           // DrawFilledRectToBuffer(18, 10, 20, 25, p);
            
            
            
            // Tell OLED to move to the start of the current page
            OLED_Command(0xB0 + p); // Set Page
            OLED_Command(0x00);     // Set lower column
            OLED_Command(0x10);     // Set higher column
            
            // Blast the 128 bytes over to the screen
            OLED_Write_Buffer(page_buffer, 128);
        }
    }
}