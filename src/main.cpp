// #define DEBUG
// #define DUMP_VRAM

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <array>
#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>


using std::array;
using std::bitset;
using std::cout;
using std::endl;
using std::string;
using std::vector;


GLuint load_and_compile_shader(std::string sname, GLenum shaderType);
GLuint create_program(std::string vertex, std::string fragment);
void initialize(GLuint &vao);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void glfwDoStuff();


GLuint vao;
GLFWwindow* window;


void cpu_read();
void cpu_write();
void cpu_tick();
void cpu_op_done();
void ppu_tick();
void ppu_render_fetches();


array<uint8_t, 256*240*3> render;
array<uint8_t, 64*3> palette = { 84, 84, 84,    0, 30,116,    8, 16,144,   48,  0,136,   68,  0,100,   92,  0, 48,   84,  4,  0,   60, 24,  0,
								 32, 42,  0,    8, 58,  0,    0, 64,  0,    0, 60,  0,    0, 50, 60,    0,  0,  0,    0,  0,  0,    0,  0,  0,
								152,150,152,    8, 76,196,   48, 50,236,   92, 30,228,  136, 20,176,  160, 20,100,  152, 34, 32,  120, 60,  0,
								 84, 90,  0,   40,114,  0,    8,124,  0,    0,118, 40,    0,102,120,    0,  0,  0,    0,  0,  0,    0,  0,  0,
								236,238,236,   76,154,236,  120,124,236,  176, 98,236,  228, 84,236,  236, 88,180,  236,106,100,  212,136, 32,
								160,170,  0,  116,196,  0,   76,208, 32,   56,204,108,   56,180,204,   60, 60, 60,    0,  0,  0,    0,  0,  0,
								236,238,236,  168,204,236,  188,188,236,  212,178,236,  236,174,236,  236,174,212,  236,180,176,  228,196,144,
								204,210,120,  180,222,120,  168,226,144,  152,226,180,  160,214,228,  160,162,160,    0,  0,  0,    0,  0,  0};

array<uint8_t, 0x10000> cpu_mem;
array<uint8_t, 0x4000> vram;
array<uint8_t, 0x100> ppu_oam;

uint16_t PC;
uint8_t rS = 0xFD;
bitset<8> rP(0x34); //0:C | 1:Z | 2:I | 3:D | 4:B | 5:1 | 6:V | 7:N

uint16_t address;
uint8_t content;
uint8_t controller_reg;
uint8_t controller_reg2; //gets glfw input after every completed cpu op. can probably happen way less
bool controller_update = false;

uint8_t fine_x = 0;
uint8_t ppuctrl = 0, ppumask = 0, ppustatus = 0, oamaddr = 0;

uint16_t scanline_h = 0, scanline_v = 0;
uint16_t ppu_address, ppu_address_latch;
uint16_t ppu_bg_low, ppu_bg_high;

bool nmi_line = false;
bool w_toggle = false;

bool ppu_odd_frame = false;
uint8_t ppu_bg_low_latch, ppu_bg_high_latch;
uint8_t ppu_nametable;
uint8_t ppu_attribute_latch;
uint32_t ppu_attribute;
uint16_t ppu_bg_address;
uint8_t ppudata_latch;
uint16_t ppu_nt_mirroring;


int main(int argc, char* argv[]) {
	if(!glfwInit()) {
		exit(1);
	}

	window = glfwCreateWindow(256, 240, "FRES++", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(1);
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) {
		std::cout << "glew died";
		glfwTerminate();
		exit(1);
	}

	initialize(vao);
	glfwSetKeyCallback(window, key_callback);


	if(argc < 2) {
		cout << "nes rom.nes" << endl;
		exit(0);
	}
	string infile = argv[1];

	std::ifstream iFile(infile.c_str(), std::ios::in | std::ios::binary);
	if(iFile.is_open() == false) {
		cout << "File not found";
		exit(0);
	}

//----- iNes stuff
	std::array<uint8_t, 16> header;
	iFile.read((char*)&header[0], 16);

	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A) {
		cout << "Not a valid .nes file" << endl;
		exit(0);
	}

	ppu_nt_mirroring = (header[6] & 1) ? ~0x400 : ~0x800; //false=v, true=h
	//where to do this? cpu r/w to ppu, everything that does stuff between ppu:0x2000-0x2FFF
//-----

	iFile.read((char*)&cpu_mem[0x8000], 0x4000);
	iFile.read((char*)&vram[0], 0x2000);
	iFile.close();
	std::copy(cpu_mem.begin()+0x8000, cpu_mem.begin()+0xC000, cpu_mem.begin()+0xC000);


	uint8_t rA = 0, rX = 0, rY = 0;
	PC = cpu_mem[0xFFFC] + (cpu_mem[0xFFFD] << 8);


	while (!glfwWindowShouldClose(window)) {
		bool alu = false;

		uint8_t opcode = cpu_mem[PC];
		uint8_t op1 = cpu_mem[PC+1];
		uint8_t op2 = cpu_mem[PC+2];

		#ifdef DEBUG
		cout << std::uppercase << std::hex << std::setfill('0')
			 << std::setw(4) << PC
			 << "  " << std::setw(2) << int(opcode)
			 << " " << std::setw(2) << int(op1)
			 << " " << std::setw(2) << int(op2)
			 << "        A:" << std::setw(2) << int(rA)
			 << " X:" << std::setw(2) << int(rX)
			 << " Y:" << std::setw(2) << int(rY)
			 << " P:" << std::setw(2) << int(rP.to_ulong() & ~0x10)
			 << " SP:" << std::setw(2) << int(rS)
			 << " PPU:" << std::setw(3) << std::dec << scanline_h
			 << " SL:" << std::setw(3) << std::dec << scanline_v << endl;
		#endif
		#ifdef DUMP_VRAM
		if(scanline_v == 261) {
			string outfile = "vram.txt";
			std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
			result.write((char*)&vram[0],0x4000);
			result.close();
		}
		#endif
		// cout << std::uppercase << std::hex << ppu_address << endl;

		++PC;
		cpu_read();

		switch(opcode) {
			// opcodes left
			// 03 07 0B 0F
			// 13 17 1B 1F
			// 23 27 2B 2F
			// 33 37 3B 3F
			// 43 47 4B 4F
			// 53 57 5B 5F
			// 63 67 6B 6F
			// 73 77 7B 7F
			// 82 83 87 89 8B 8F
			// 93 97 9B 9C 9E 9F
			// A3 A7 AB AF
			// B3 B7 BB BF
			// C2 C3 C7 CB CF
			// D3 D7 DB DF
			// E2 E3 E7 EB EF
			// F3 F7 FB FF

			case 0x00: //BRK
				++PC;
				cpu_read();
				cpu_mem[0x100+rS] = PC >> 8;
				--rS;
				cpu_write();
				cpu_mem[0x100+rS] = PC;
				--rS;
				cpu_write();
				rP.set(4);
				cpu_mem[0x100+rS] = rP.to_ulong();
				--rS;
				cpu_write();
				PC = cpu_mem[0xFFFE];
				rP.set(2);
				cpu_read();
				PC += cpu_mem[0xFFFF] << 8;
				cpu_read();
				break;
			case 0x06: //ASL
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1] & 0x80;
				cpu_mem[op1] <<= 1;
				rP[1] = !cpu_mem[op1];
				rP[7] = cpu_mem[op1] & 0x80;
				cpu_write();
				break;
			case 0x08: //PHP
				cpu_read();
				rP.set(4);
				cpu_mem[0x100+rS] = rP.to_ulong();
				--rS;
				cpu_write();
				break;
			case 0x0A: //ASL
				cpu_read();
				rP[0] = rA & 0x80;
				rA <<= 1;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;
			case 0x0E: //ASL
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_mem[op1 + (op2 << 8)] <<= 1;
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_write();
				break;
			case 0x10: //BPL
				++PC;
				cpu_read();
				if(!rP.test(7)) {
					uint8_t prevPCH = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPCH) {
						cpu_read();
					}
				}
				break;
			case 0x16: //ASL
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_mem[op1 + rX & 0xFF] <<= 1;
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_write();
				break;
			case 0x18: //CLC
				cpu_read();
				rP.reset(0);
				break;
			case 0x1E: //ASL
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] <<= 1;
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_write();
				break;
			case 0x20: //JSR
				++PC;
				cpu_read();
				cpu_read();
				cpu_mem[0x100+rS] = PC >> 8;
				--rS;
				cpu_write();
				cpu_mem[0x100+rS] = PC;
				--rS;
				cpu_write();
				PC = op1 + (op2 << 8);
				cpu_read();
				break;
			case 0x26: //ROL
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1];
				cpu_mem[op1] = (cpu_mem[op1] << 1) | rP[0];
				rP[0] = prevMem & 0x80;
				}
				rP[1] = !cpu_mem[op1];
				rP[7] = cpu_mem[op1] & 0x80;
				cpu_write();
				break;
			case 0x28: //PLP
				cpu_read();
				++rS;
				cpu_read();
				rP = cpu_mem[0x100+rS] | 0x20;
				cpu_read();
				break;
			case 0x2A: //ROL
				cpu_read();
				{
				uint8_t prevrA = rA;
				rA = (rA << 1) | rP[0];
				rP[0] = prevrA & 0x80;
				}
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;
			case 0x2E: //ROL
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + (op2 << 8)];
				cpu_mem[op1 + (op2 << 8)] = (cpu_mem[op1 + (op2 << 8)] << 1) | rP[0];
				rP[0] = prevMem & 0x80;
				}
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_write();
				break;
			case 0x30: //BMI
				++PC;
				cpu_read();
				if(rP.test(7)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0x36: //ROL
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + rX & 0xFF];
				cpu_mem[op1 + rX & 0xFF] = (cpu_mem[op1 + rX & 0xFF] << 1) | rP[0];
				rP[0] = prevMem & 0x80;
				}
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_write();
				break;
			case 0x38: //SEC
				cpu_read();
				rP.set(0);
				break;
			case 0x3E: //ROL
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] = (cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] << 1) | rP[0];
				rP[0] = prevMem & 0x80;
				}
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_write();
				break;
			case 0x40: //RTI
				cpu_read();
				++rS;
				cpu_read();
				rP = cpu_mem[0x100+rS] | 0x20;
				++rS;
				cpu_read();
				PC = cpu_mem[0x100+rS];
				++rS;
				cpu_read();
				PC += cpu_mem[0x100+rS] << 8;
				cpu_read();
				break;
			case 0x46: //LSR
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1] & 0x01;
				cpu_mem[op1] >>= 1;
				rP[1] = !cpu_mem[op1];
				rP.reset(7);
				cpu_write();
				break;
			case 0x48: //PHA
				cpu_read();
				cpu_mem[0x100+rS] = rA; //can do rS--
				--rS;
				cpu_write();
				break;
			case 0x4A: //LSR
				cpu_read();
				rP[0] = rA & 0x01;
				rA >>= 1;
				rP[1] = !rA;
				rP.reset(7);
				break;
			case 0x4C: //JMP
				++PC;
				cpu_read();
				PC = op1 + (op2 << 8);
				cpu_read();
				break;
			case 0x4E: //LSR
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + (op2 << 8)] & 0x01;
				cpu_mem[op1 + (op2 << 8)] >>= 1;
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP.reset(7);
				cpu_write();
				break;
			case 0x50: //BVC
				++PC;
				cpu_read();
				if(!rP.test(6)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0x56: //LSR
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + rX & 0xFF] & 0x01;
				cpu_mem[op1 + rX & 0xFF] >>= 1;
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP.reset(7);
				cpu_write();
				break;
			case 0x5E: //LSR
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				rP[0] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x01;
				cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] >>= 1;
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP.reset(7);
				cpu_write();
				break;
			case 0x60: //RTS
				cpu_read();
				++rS;
				cpu_read();
				PC = cpu_mem[0x100+rS];
				++rS;
				cpu_read();
				PC += cpu_mem[0x100+rS] << 8;
				cpu_read();
				++PC;
				cpu_read();
				break;
			case 0x66: //ROR
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1];
				cpu_mem[op1] = (cpu_mem[op1] >> 1) | (rP[0] << 7);
				rP[0] = prevMem & 0x01;
				}
				rP[1] = !cpu_mem[op1];
				rP[7] = cpu_mem[op1] & 0x80;
				cpu_write();
				break;
				case 0x68: //PLA
				cpu_read();
				++rS;
				cpu_read();
				rA = cpu_mem[0x100+rS];
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				cpu_read();
				break;
			case 0x6A: //ROR
				cpu_read();
				{
				uint8_t prevrA = rA;
				rA = (rA >> 1) | (rP[0] << 7);
				rP[0] = prevrA & 0x01;
				}
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;
			case 0x6C: //JMP
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				PC = cpu_mem[op1 + (op2 << 8)];
				PC += cpu_mem[(op1 + 1 & 0xFF) + (op2 << 8)] << 8;
				cpu_read();
				break;
			case 0x6E: //ROR
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + (op2 << 8)];
				cpu_mem[op1 + (op2 << 8)] = (cpu_mem[op1 + (op2 << 8)] >> 1) | (rP[0] << 7);
				rP[0] = prevMem & 0x01;
				}
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_write();
				break;
			case 0x70: //BVS
				++PC;
				cpu_read();
				if(rP.test(6)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0x76: //ROR
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + rX & 0xFF];
				cpu_mem[op1 + rX & 0xFF] = (cpu_mem[op1 + rX & 0xFF] >> 1) | (rP[0] << 7);
				rP[0] = prevMem & 0x01;
				}
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_write();
				break;
			case 0x78: //SEI
				cpu_read();
				rP.set(2);
				break;
			case 0x7E: //ROR
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				{
				uint8_t prevMem = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] = (cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] >> 1) | (rP[0] << 7);
				rP[0] = prevMem & 0x01;
				}
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_write();
				break;
			case 0x81: //STA
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_read();
				address = cpu_mem[op1 + rX & 0xFF] + (cpu_mem[op1 + rX + 1 & 0xFF] << 8);
				content = rA;
				cpu_mem[address] = rA;
				cpu_write();
				break;
			case 0x84: //STY
				++PC;
				cpu_read();
				cpu_mem[op1] = rY;
				cpu_write();
				break;
			case 0x85: //STA
				++PC;
				cpu_read();
				cpu_mem[op1] = rA;
				cpu_write();
				break;
			case 0x86: //STX
				++PC;
				cpu_read();
				cpu_mem[op1] = rX;
				cpu_write();
				break;
			case 0x88: //DEY
				cpu_read();
				--rY;
				rP[1] = !rY;
				rP[7] = rY & 0x80;
				break;
			case 0x8A: //TXA
				cpu_read();
				rA = rX;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;
			case 0x8C: //STY
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = op1 + (op2 << 8);
				content = rY;
				cpu_mem[address] = rY;
				cpu_write();
				break;
			case 0x8D: //STA
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = op1 + (op2 << 8);
				content = rA;
				cpu_mem[address] = rA;
				cpu_write();
				break;
			case 0x8E: //STX
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = op1 + (op2 << 8);
				content = rX;
				cpu_mem[op1 + (op2 << 8)] = rX;
				cpu_write();
				break;
			case 0x90: //BCC
				++PC;
				cpu_read();
				if(!rP.test(0)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0x91: //STA
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_read();
				address = cpu_mem[op1] + (cpu_mem[op1 + 1 & 0xFF] << 8) + rY;
				content = rA;
				cpu_mem[address] = rA;
				cpu_write();
				break;
			case 0x94: //STY
				++PC;
				cpu_read();
				cpu_read();
				cpu_mem[op1 + rX & 0xFF] = rY;
				cpu_write();
				break;
			case 0x95: //STA
				++PC;
				cpu_read();
				cpu_read();
				cpu_mem[op1 + rX & 0xFF] = rA;
				cpu_write();
				break;
			case 0x96: // STX
				++PC;
				cpu_read();
				cpu_read();
				cpu_mem[op1 + rY & 0xFF] = rX;
				cpu_write();
				break;
			case 0x98: //TYA
				cpu_read();
				rA = rY;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;
			case 0x99: //STA
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				address = op1 + (op2 << 8) + rY;
				content = rA;
				cpu_mem[address] = rA;
				cpu_write();
				break;
			case 0x9A: //TXS
				cpu_read();
				rS = rX;
				break;
			case 0x9D: //STA
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				address = op1 + (op2 << 8) + rX;
				content = rA;
				cpu_mem[address] = rA;
				cpu_write();
				break;
			case 0xA8: //TAY
				cpu_read();
				rY = rA;
				rP[1] = !rY;
				rP[7] = rY & 0x80;
				break;
			case 0xAA: //TAX
				cpu_read();
				rX = rA;
				rP[1] = !rX;
				rP[7] = rX & 0x80;
				break;
			case 0xB0: //BCS
				++PC;
				cpu_read();
				if(rP.test(0)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0xB8: //CLV
				cpu_read();
				rP.reset(6);
				break;
			case 0xBA: //TSX
				cpu_read();
				rX = rS;
				rP[1] = !rX;
				rP[7] = rX & 0x80;
				break;
			case 0xC6: //DEC
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				--cpu_mem[op1];
				rP[1] = !cpu_mem[op1];
				rP[7] = cpu_mem[op1] & 0x80;
				cpu_write();
				break;
			case 0xC8: //INY
				cpu_read();
				++rY;
				rP[1] = !rY;
				rP[7] = rY & 0x80;
				break;
			case 0xCA: //DEX
				cpu_read();
				--rX;
				rP[1] = !rX;
				rP[7] = rX & 0x80;
				break;
			case 0xCE: //DEC
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				--cpu_mem[op1 + (op2 << 8)];
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_write();
				break;
			case 0xD0: //BNE
				++PC;
				cpu_read();
				if(!rP.test(1)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0xD6: //DEC
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				--cpu_mem[op1 + rX & 0xFF];
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_write();
				break;
			case 0xD8: //CLD
				cpu_read();
				rP.reset(3);
				break;
			case 0xDE: //DEC
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				--cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_write();
				break;
			case 0xE6: //INC
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				++cpu_mem[op1];
				rP[1] = !cpu_mem[op1];
				rP[7] = cpu_mem[op1] & 0x80;
				cpu_write();
				break;
			case 0xE8: //INX
				cpu_read();
				++rX;
				rP[1] = !rX;
				rP[7] = rX & 0x80;
				break;
			case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA: //NOP
				cpu_read();
				break;
			case 0xEE: //INC
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_write();
				++cpu_mem[op1 + (op2 << 8)];
				rP[1] = !cpu_mem[op1 + (op2 << 8)];
				rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
				cpu_write();
				break;
			case 0xF0: //BEQ
				++PC;
				cpu_read();
				if(rP.test(1)) {
					uint8_t prevPC = PC >> 8;
					PC += int8_t(op1);
					cpu_read();
					if((PC >> 8) != prevPC) {
						cpu_read();
					}
				}
				break;
			case 0xF6: //INC
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				++cpu_mem[op1 + rX & 0xFF];
				rP[1] = !cpu_mem[op1 + rX & 0xFF];
				rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
				cpu_write();
				break;
			case 0xF8: //SED
				cpu_read();
				rP.set(3);
				break;
			case 0xFE: //INC
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_write();
				++cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
				rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
				cpu_write();
				break;

			case 0x01: case 0x21: case 0x41: case 0x61:
			case 0xA1: case 0xC1: case 0xE1: //(indirect,x) / indexed indirect
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				cpu_read();
				address = cpu_mem[op1 + rX & 0xFF] + (cpu_mem[op1 + rX + 1 & 0xFF] << 8);
				content = cpu_mem[address];
				cpu_read();
				alu = true;
				break;

			case 0x04: case 0x05: case 0x24: case 0x25: case 0x44: //zero page
			case 0x45: case 0x64: case 0x65: case 0xA4: case 0xA5:
			case 0xA6: case 0xC4: case 0xC5: case 0xE4: case 0xE5:
				++PC;
				cpu_read();
				content = cpu_mem[op1];
				cpu_read();
				alu = true;
				break;

			case 0x09: case 0x29: case 0x49: case 0x69: case 0x80: case 0xA0: //immediate
			case 0xA2: case 0xA9: case 0xC0: case 0xC9: case 0xE0: case 0xE9:
				++PC;
				cpu_read();
				content = op1;
				alu = true;
				break;

			case 0x0C: case 0x0D: case 0x2C: case 0x2D: case 0x4D: case 0x6D: //absolute
			case 0xAC: case 0xAD: case 0xAE: case 0xCC: case 0xCD: case 0xEC: case 0xED:
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = op1 + (op2 << 8);
				content = cpu_mem[address];
				cpu_read();
				alu = true;
				break;

			case 0x11: case 0x31: case 0x51: //(indirect),y / indirect indexed
			case 0x71: case 0xB1: case 0xD1: case 0xF1:
				++PC;
				cpu_read();
				cpu_read();
				cpu_read();
				address = (cpu_mem[op1] + rY & 0xFF) + (cpu_mem[op1 + 1 & 0xFF] << 8);
				content = cpu_mem[address];
				cpu_read();
				if((cpu_mem[op1] + rY) > 0xFF) {
					address = cpu_mem[op1] + (cpu_mem[op1 + 1 & 0xFF] << 8) + rY; // address = address + 0x100 & 0xFFFF; //address gets reset. recalculate
					content = cpu_mem[address];
					cpu_read();
				}
				alu = true;
				break;

			case 0x14: case 0x15: case 0x34: case 0x35: //zero page,X
			case 0x54: case 0x55: case 0x74: case 0x75: case 0xB4:
			case 0xB5: case 0xD4: case 0xD5: case 0xF4: case 0xF5:
				++PC;
				cpu_read();
				cpu_read();
				content = cpu_mem[op1 + rX & 0xFF];
				cpu_read();
				alu = true;
				break;

			case 0x19: case 0x39: case 0x59: case 0x79: //absolute,Y
			case 0xB9: case 0xBE: case 0xD9: case 0xF9:
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = (op1 + rY & 0xFF) + (op2 << 8);
				content = cpu_mem[address];
				cpu_read();
				if((op1 + rY) > 0xFF) {
					address = op1 + (op2 << 8) + rY;
					content = cpu_mem[address];
					cpu_read();
				}
				alu = true;
				break;

			case 0x1C: case 0x1D: case 0x3C: case 0x3D: //absolute,X
			case 0x5C: case 0x5D: case 0x7C: case 0x7D: case 0xBC:
			case 0xBD: case 0xDC: case 0xDD: case 0xFC: case 0xFD:
				++PC;
				cpu_read();
				++PC;
				cpu_read();
				address = (op1 + rX & 0xFF) + (op2 << 8);
				content = cpu_mem[address];
				cpu_read();
				if((op1 + rX) > 0xFF) {
					address = op1 + (op2 << 8) + rX;
					content = cpu_mem[address];
					cpu_read();
				}
				alu = true;
				break;

			case 0xB6: //zero page,Y
				++PC;
				cpu_read();
				cpu_read();
				content = cpu_mem[op1 + rY & 0xFF];
				cpu_read();
				alu = true;
				break;

			default:
				cout << "MELTDOWN " << std::hex << int(opcode) << endl;
				exit(0);
		}

		if(alu) {
			switch(opcode) {
				case 0x61: case 0x65: case 0x69: case 0x6D:
				case 0x71: case 0x75: case 0x79: case 0x7D: //adc
					{
					uint8_t prevrA = rA;
					rA += content + rP[0];
					rP[0] = (prevrA + content + rP[0]) >> 8;
					rP[1] = !rA;
					rP[6] = (prevrA ^ rA) & (content ^ rA) & 0x80;
					}
					rP[7] = rA & 0x80;
					break;

				case 0x21: case 0x25: case 0x29: case 0x2D:
				case 0x31: case 0x35: case 0x39: case 0x3D: //and
					rA &= content;
					rP[1] = !rA;
					rP[7] = rA & 0x80;
					break;

				case 0xC1: case 0xC5: case 0xC9: case 0xCD:
				case 0xD1: case 0xD5: case 0xD9: case 0xDD: //cmp
					rP[0] = (rA >= content) ? 1 : 0;
					rP[1] = !(rA - content);
					rP[7] = rA - content & 0x80;
					break;

				case 0x41: case 0x45: case 0x49: case 0x4D:
				case 0x51: case 0x55: case 0x59: case 0x5D: //eor
					rA ^= content;
					rP[1] = !rA;
					rP[7] = rA & 0x80;
					break;

				case 0xA1: case 0xA5: case 0xA9: case 0xAD:
				case 0xB1: case 0xB5: case 0xB9: case 0xBD: //lda
					rA = content;
					rP[1] = !rA;
					rP[7] = rA & 0x80;
					break;

				case 0x01: case 0x05: case 0x09: case 0x0D:
				case 0x11: case 0x15: case 0x19: case 0x1D: //ora
					rA |= content;
					rP[1] = !rA;
					rP[7] = rA & 0x80;
					break;

				case 0xE1: case 0xE5: case 0xE9: case 0xED:
				case 0xF1: case 0xF5: case 0xF9: case 0xFD: //sbc
					{
					uint8_t prevrA = rA;
					rA = (rA - content) - !rP[0];
					rP[0] = ((prevrA - content) - !rP[0] < 0) ? 0 : 1;
					rP[1] = !rA;
					rP[6] = (prevrA ^ rA) & (~content ^ rA) & 0x80;
					}
					rP[7] = rA & 0x80;
					break;

				case 0xE0: case 0xE4: case 0xEC: //cpx
					rP[0] = (rX >= content) ? 1 : 0;
					rP[1] = !(rX - content);
					rP[7] = rX - content & 0x80;
					break;

				case 0xC0: case 0xC4: case 0xCC: //cpy
					rP[0] = (rY >= content) ? 1 : 0;
					rP[1] = !(rY - content);
					rP[7] = rY - content & 0x80;
					break;

				case 0xA2: case 0xA6: case 0xAE: case 0xB6: case 0xBE: //ldx
					rX = content;
					rP[1] = !rX;
					rP[7] = rX & 0x80;
					break;

				case 0xA0: case 0xA4: case 0xAC: case 0xB4: case 0xBC: //ldy
					rY = content;
					rP[1] = !rY;
					rP[7] = rY & 0x80;
					break;

				case 0x24: case 0x2C: //bit
					rP[1] = !(content & rA);
					rP[6] = content & 0x40;
					rP[7] = content & 0x80;
					break;
			}
		}
		cpu_op_done();

		if(controller_update) {
			controller_reg = controller_reg2;
		}
	}

	glfwTerminate();
	return 0;
}


void cpu_read() {
	switch(address) {
		case 0x2002:
			content = ppustatus;
			ppustatus &= 0x7F;
			// cpu_mem[0x2002] = ppustatus; //probably not necessary
			w_toggle = false;
			break;
		case 0x2007:
			if((scanline_v >= 240 && scanline_v <= 260) || !(ppumask & 0x18)) {
				//if in palette range, reload latch(mirrored) but send palette byte immediately
				//perform NT mirroring here
				content = ppudata_latch;
				ppudata_latch = vram[ppu_address];
				ppu_address += (ppuctrl & 0x04) ? 0x20 : 0x01;
			}
			break;

		case 0x4016:
			content = (address & 0xE0) | (controller_reg & 1); //address = open bus?
			controller_reg >>= 1; //if we have read 4016 8 times, start returning 1s
			break;
	}

	cpu_tick();
	return;
}


void cpu_write() { // block writes to ppu on the first screen
	switch(address) {
		case 0x2000:
			ppuctrl = content;
			ppu_address_latch = (ppu_address_latch & 0x73FF) | ((ppuctrl & 0x03) << 10);
			break;
		case 0x2001:
			ppumask = content;
			break;
		case 0x2005:
			if(!w_toggle) {
				ppu_address_latch = (ppu_address_latch & 0x7FE0) | (content >> 3);
				fine_x = content & 0x07;
			} else {
				ppu_address_latch = (ppu_address_latch & 0xC1F) | ((content & 0x07) << 12) | ((content & 0xF8) << 2);
			}
			w_toggle = !w_toggle;
			break;
		case 0x2006:
			if(!w_toggle) {
				ppu_address_latch = (ppu_address_latch & 0xFF) | ((content & 0x3F) << 8);
			} else {
				ppu_address_latch = (ppu_address_latch & 0x7F00) | content;
				ppu_address = ppu_address_latch & 0x3FFF; //assuming only the address gets mirrored down, not the latch
			}
			w_toggle = !w_toggle;
			break;
		case 0x2007:
			if((scanline_v >= 240 && scanline_v <= 260) || !(ppumask & 0x18)) {
				vram[ppu_address] = content; //perform NT mirroring here
				ppu_address += (ppuctrl & 0x04) ? 0x20 : 0x01;
			}
			break;
		case 0x4014:
			// the cpu writes to oamdata (0x2004) 256 times, the cpu is suspended during the transfer.

			// if(cycles & 1) cpu_tick(); //+1 cycle if dma started on an odd cycle
			// cpu_tick();
			for(uint16_t x=0; x<=255; ++x) {
				// cpu_tick(); //r:0x20xx
				ppu_oam[x] = cpu_mem[(content << 8) + x];
				++oamaddr;
				// cpu_tick(); //w:0x4014
			}
			break;
		case 0x4016:
			if(content & 1) {
				controller_update = true;
			} else /*if(!(content & 1))*/ {
				controller_update = false;
			}
			break;
	}

	cpu_tick();
	return;
}


void cpu_tick() {
	ppu_tick();
	ppu_tick();
	ppu_tick();

	address = 0; //really needed?

	return;
}


void cpu_op_done() {
	if(nmi_line) {
		// ++PC;
		// cpu_read();
		cpu_mem[0x100+rS] = PC >> 8;
		--rS;
		// cpu_write();
		cpu_mem[0x100+rS] = PC;
		--rS;
		// cpu_write();
		rP.reset(4);
		cpu_mem[0x100+rS] = rP.to_ulong();
		--rS;
		// cpu_write();
		PC = cpu_mem[0xFFFA];
		rP.set(2);
		// cpu_read();
		PC += cpu_mem[0xFFFB] << 8;
		// cpu_read();

		nmi_line = false;
	}

	return;
}


void ppu_tick() {
	if(scanline_v < 240) { //visible scanlines
		if(scanline_v != 261 && scanline_h && scanline_h <= 256) {
			uint8_t ppu_pixel = (ppu_bg_low >> (15 - fine_x)) & 1;
			ppu_pixel |= (ppu_bg_high >> (14 - fine_x)) & 2;
			ppu_pixel |= (ppu_attribute >> (28 - fine_x * 2)) & 0x0C;

			uint32_t renderpos = (scanline_v*256 + (scanline_h-1)) * 3;
			uint16_t pindex = vram[0x3F00 + ppu_pixel] * 3;
			render[renderpos  ] = palette[pindex  ];
			render[renderpos+1] = palette[pindex+1];
			render[renderpos+2] = palette[pindex+2];
		}

		ppu_render_fetches();

	} else if(scanline_v == 241) { //vblank scanlines
		if(scanline_h == 1) {
			ppustatus |= 0x80;
			nmi_line = cpu_mem[0x2000] & ppustatus & 0x80;

			glfwDoStuff();
		}
	} else if(scanline_v == 261) { //prerender scanline
		if(scanline_h == 1) {
			ppustatus &= 0x1F; //clear sprite overflow, sprite 0 hit and vblank
		} else if(scanline_h >= 280 && scanline_h <= 304 && ppumask & 0x18) {
			ppu_address = (ppu_address & 0x41F) | (ppu_address_latch & 0x7BE0);
		}

		ppu_render_fetches();

		if(ppu_odd_frame && scanline_h == 339) {
			++scanline_h;
		}
	}

	if(scanline_h == 340) {
		scanline_h = 0;
		if(scanline_v == 261) {
			scanline_v = 0;
			ppu_odd_frame = !ppu_odd_frame;
		} else {
			++scanline_v;
		}
	} else {
		++scanline_h;
	}

	return;
}


void ppu_render_fetches() { //things done during visible and prerender scanlines
	if(ppumask & 0x18) { //if rendering is enabled
		if(scanline_h == 256) { //Y increment
			if(ppu_address < 0x7000) {
				ppu_address += 0x1000;
			} else {
				ppu_address &= 0xFFF;
				if((ppu_address & 0x3E0) == 0x3A0) {
					ppu_address &= 0xC1F;
					ppu_address ^= 0x800;
				} else if((ppu_address & 0x3E0) == 0x3E0) {
					ppu_address &= 0xC1F;
				} else {
					ppu_address += 0x20;
				}
			}
		}

		if(scanline_h == 257) {
			ppu_address = (ppu_address & 0x7BE0) | (ppu_address_latch & 0x41F);
		}

		if((scanline_h >= 1 && scanline_h <= 257) || (scanline_h >= 321 && scanline_h <= 339)) {
			if(!(scanline_h & 0x07)) { //coarse X increment
				if((ppu_address & 0x1F) != 0x1F) {
					++ppu_address;
				} else {
					ppu_address = (ppu_address & 0x7FE0) ^ 0x400;
				}
			}

			switch(scanline_h & 0x07) {
				case 1:
					if(scanline_h != 1 && scanline_h != 321) {
						ppu_bg_low |= ppu_bg_low_latch;
						ppu_bg_high |= ppu_bg_high_latch;
						ppu_attribute |= (ppu_attribute_latch & 0x03) * 0x5555; //2-bit splat
					}
					ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					break;
				case 3:
					if(scanline_h != 339) {
						ppu_attribute_latch = vram[0x23C0 | (ppu_address & 0xC00) | (ppu_address >> 4 & 0x38) | (ppu_address >> 2 & 0x07)];
						ppu_attribute_latch >>= (((ppu_address >> 1) & 0x01) | ((ppu_address >> 5) & 0x02)) * 2;
					} else {
						ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					}
					break;
				case 5:
					ppu_bg_address = (ppu_nametable*16 + (ppu_address >> 12)) | ((ppuctrl & 0x10) << 8); //tile X is correct, but Y is always 0.
					ppu_bg_low_latch = vram[ppu_bg_address];
					break;
				case 7:
					ppu_bg_high_latch = vram[ppu_bg_address + 8];
					break;
			}
		}

		if((scanline_h >= 1 && scanline_h <= 256) || (scanline_h >= 321 && scanline_h <= 336)) {
			ppu_bg_low <<= 1;
			ppu_bg_high <<= 1;
			ppu_attribute <<= 2;
		}

	}

	if(scanline_h >= 257 && scanline_h <= 320) { //rendering enabled only?
		oamaddr = 0;
	}

	return;
}


GLuint load_and_compile_shader(std::string sname, GLenum shaderType) {
	vector<char> buffer(sname.begin(), sname.end());
	buffer.push_back(0);
	const char *src = &buffer[0];

	// Compile the shader
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	// Check the result of the compilation
	GLint test;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
	if(!test) {
		std::cerr << "Shader compilation failed with this message:" << std::endl;
		vector<char> compilation_log(512);
		glGetShaderInfoLog(shader, compilation_log.size(), NULL, &compilation_log[0]);
		std::cerr << &compilation_log[0] << std::endl;
		glfwTerminate();
		exit(-1);
	}
	return shader;
}


GLuint create_program(std::string vertex, std::string fragment) {
	// Load and compile the vertex and fragment shaders
	GLuint vertexShader = load_and_compile_shader(vertex, GL_VERTEX_SHADER);
	GLuint fragmentShader = load_and_compile_shader(fragment, GL_FRAGMENT_SHADER);

	// Attach the above shader to a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Flag the shaders for deletion
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Link and use the program
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	return shaderProgram;
}


void initialize(GLuint &vao) {
	// Use a Vertex Array Object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// 1 square (made by 2 triangles) to be rendered
	GLfloat vertices_position[8] = {
		-1.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f,
		-1.0f, -1.0f		
	};

	GLfloat texture_coord[8] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	GLuint indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	std::string vertex = "#version 150\n in vec4 position; in vec2 texture_coord; out vec2 texture_coord_from_vshader; void main() {gl_Position = position; texture_coord_from_vshader = texture_coord;}";
	std::string fragment = "#version 150\n in vec2 texture_coord_from_vshader; out vec4 out_color; uniform sampler2D texture_sampler; void main() {out_color = texture(texture_sampler, texture_coord_from_vshader);}";

	// Create a Vector Buffer Object that will store the vertices on video memory
	GLuint vbo;
	glGenBuffers(1, &vbo);

	// Allocate space for vertex positions and texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_position) + sizeof(texture_coord), NULL, GL_STATIC_DRAW);

	// Transfer the vertex positions:
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_position), vertices_position);

	// Transfer the texture coordinates:
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices_position), sizeof(texture_coord), texture_coord);

	// Create an Element Array Buffer that will store the indices array:
	GLuint eab;
	glGenBuffers(1, &eab);

	// Transfer the data from indices to eab
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Create a texture
	GLuint texture;
	glGenTextures(1, &texture);

	// Specify that we work with a 2D texture
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)&render[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLuint shaderProgram = create_program(vertex, fragment);

	GLint position_attribute = glGetAttribLocation(shaderProgram, "position");

	// Specify how the data for position can be accessed
	glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);

	// Texture coord attribute
	GLint texture_coord_attribute = glGetAttribLocation(shaderProgram, "texture_coord");
	glVertexAttribPointer(texture_coord_attribute, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)sizeof(vertices_position));
	glEnableVertexAttribArray(texture_coord_attribute);
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	controller_reg2 = 0;
	controller_reg2 |= (key == GLFW_KEY_S     && action == GLFW_PRESS) ? 0x01 : 0;
	controller_reg2 |= (key == GLFW_KEY_A     && action == GLFW_PRESS) ? 0x02 : 0;
	controller_reg2 |= (key == GLFW_KEY_W     && action == GLFW_PRESS) ? 0x04 : 0;
	controller_reg2 |= (key == GLFW_KEY_Q     && action == GLFW_PRESS) ? 0x08 : 0;
	controller_reg2 |= (key == GLFW_KEY_UP    && action == GLFW_PRESS) ? 0x10 : 0;
	controller_reg2 |= (key == GLFW_KEY_DOWN  && action == GLFW_PRESS) ? 0x20 : 0;
	controller_reg2 |= (key == GLFW_KEY_LEFT  && action == GLFW_PRESS) ? 0x40 : 0;
	controller_reg2 |= (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) ? 0x80 : 0;

	return;
}


void glfwDoStuff() {
	glClear(GL_COLOR_BUFFER_BIT);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)&render[0]);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glfwSwapBuffers(window);

	/* Poll for and process events */
	glfwPollEvents();
	return;
}
