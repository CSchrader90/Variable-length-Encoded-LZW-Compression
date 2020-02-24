#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <cmath>

#define BYTE_BITS 8
#define PAGE_SIZE 4096
#define MAX_OUT_BITS 14
#define MAX_CODE pow(2, MAX_OUT_BITS) //maximum code to be added to dictionaries

using namespace std;
typedef unordered_map<string, unsigned int> com_dict; //compressor dictionary
typedef unordered_map<unsigned int, string> dec_dict; //decompressor dictionary

//Variable bit-length output
void output_bits(unsigned int num_b, unsigned int& used_bits, \
   unsigned char& out_B, com_dict& dict, string w, ofstream& output) {

   int unwritten;
   int written = 0;
   
   out_B |= dict[w] >> (used_bits + num_b % BYTE_BITS);
   output << out_B;
   written += (BYTE_BITS - used_bits);
   unwritten = num_b - written;
   out_B = '\0';

   if (unwritten > BYTE_BITS) {
      out_B |= dict[w] >> unwritten % BYTE_BITS;
      output << out_B;
      out_B = '\0';
      written += BYTE_BITS;
      unwritten = num_b - written;
   }
   out_B |= dict[w] << (BYTE_BITS - unwritten);
   used_bits = unwritten;
}

void compress(ifstream& input, ofstream& output)
{
   com_dict dict; //compressor dictionary
   for (int i = 0; i < 256; i++) {
      dict[string(1, (char)i)] = i;
   }
   unsigned int code = 256;
   unsigned char out_B = '\0'; //byte holding output
   unsigned int num_b = 9; //bit length of current output codes
   unsigned int used_bits = 0; //bits used up in current output byte
   string w = "";
   string wk = "";
   char k;

   while (input.get(k)) {
      wk = w + k;
      if (dict.find(wk) == dict.end()) { //if string not found in dict

         if (code < MAX_CODE) dict[wk] = code++;
         
         if (used_bits == BYTE_BITS) {
            output << out_B;
            used_bits = 0;
            out_B = '\0';
         }           

         output_bits(num_b, used_bits, out_B, dict, w, output);

         if (code == pow(2, num_b)) {
            num_b++;
         }
      
         w = k;
      }
      else {
         w = wk;
      }
   }

   output_bits(num_b, used_bits, out_B, dict, w, output);
   output << out_B;

}

bool get_code(ifstream& input, unsigned int& cur, int& read_bits, int& num_b, int& excess, unsigned int& code) {
   char in_buf;
      
   if (input.get(in_buf)) {
      read_bits += BYTE_BITS;
      cur |= (unsigned char)in_buf;

      while (read_bits  < num_b) {
         cur = cur << BYTE_BITS;
         input.get(in_buf);
         read_bits += BYTE_BITS;
         cur |= (unsigned char)in_buf;
      }
      excess = read_bits - num_b;
      cur = cur >> excess;
      code = cur;
      cur = in_buf & (unsigned char)(pow(2, excess) - 1);
      read_bits = excess;

      if (excess < num_b) cur = cur << 8;
      return true;
   }
   else {
      return false;
   }
}

void decompress(ifstream& input, ofstream& output) {
   dec_dict dict; 
   unsigned int cur = 0; //current bit array used to extract the required num_b bits 

   unsigned int rec_code; //received code
   unsigned int next_code;
   string w;
   int num_b = 9;
   int read_bits = 0;
   int excess;

   for (next_code = 0; next_code < 256; next_code++) {
      dict[next_code] = string(1, (char)next_code);
   }

   get_code(input, cur, read_bits, num_b, excess, rec_code);

   output << dict[rec_code];
   w = dict[rec_code];

   while (get_code(input, cur, read_bits, num_b, excess, rec_code)) {

      if (dict.find(rec_code) == dict.end()) {
         dict[rec_code] = w + w[0];
      }

      output << dict[rec_code];

      if (next_code < MAX_CODE) dict[next_code++] = w + dict[rec_code][0];

      w = dict[rec_code];
      
      if (next_code == pow(2, num_b)-1) {
         num_b++;
      }
   }
}

int main(int argc, char** argv)
{
   if (argc != 4 || (string(argv[1]).compare("-c") && string(argv[1]).compare("-d"))) {
      cout << "usage: -c/-d input_file_name output_file_name" << endl;
      return -1;
   }

   ifstream input(argv[2], ifstream::in);
   if (input.fail()) {
      cout << "Unable to open input file" << endl;
      return 0;
   }
   ofstream output(argv[3], ofstream::out);
   if (output.fail()) {
      cout << "Unable to open output file" << endl;
      return 0;
   }

   if (!string(argv[1]).compare("-c")) {
      compress(input, output);
   }
   else {
      decompress(input, output);
   }

   return 0;
}