
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <string>
#include <sstream>

#define TAPE_LEN 65536

class BF {
public:
    uint8_t data[TAPE_LEN];
    uint8_t instr[TAPE_LEN];
    uint16_t addr_pointer;
    uint16_t instr_pointer;
    int max_instr_pointer;

    BF(){
        for (int i = 0; i < TAPE_LEN; i++) data[i] = i % 64;
        addr_pointer = 0;
        instr_pointer = 0;
        max_instr_pointer = 0;
    }

    void get_data_at_addr(int addr, uint8_t* res){
        *res = data[addr];
    }

    void get_data_in_range(uint16_t addr_lo, uint16_t addr_hi, uint8_t res[]){
        for (int a = addr_lo; a <= addr_hi; a++){
            res[a-addr_lo] = data[a];
        }
    }

    void load_program(std::string program){
        for(int i=0; i < program.length(); i++){
            instr[i] = program[i];
        }
        max_instr_pointer = program.length();
    }

    void step(){
        if (instr_pointer == max_instr_pointer) return;
        const char next_instr = (char)instr[instr_pointer];
        switch(next_instr){
            case '+': inp_inc();break;
            case '-': inp_dec();break;
            case '<': inp_sub();break;
            case '>': inp_add();break;
            case ',': {
                uint8_t out =  inp_out();
                //handle display here later
                std::cout << out << std::endl;
                break;
            }
            case '.': {
                uint8_t v;
                std::cin >> v;
                inp_in(v);
                break;
            }
            case '[': inp_l_start();break;
            case ']': inp_l_end();break;
            default: instr_pointer++; //don't care about others
        }
    }

public: //BF instructions
    void inp_inc(){
        addr_pointer++;
        instr_pointer++;
    }
    void inp_dec(){
        addr_pointer--;
        instr_pointer++;
    }
    void inp_add(){
        data[addr_pointer]++;
        instr_pointer++;
    }
    void inp_sub(){
        data[addr_pointer]--;
        instr_pointer++;
    }
    uint8_t inp_out(){
        instr_pointer++;
        return data[addr_pointer];
    }
    void inp_in(uint8_t in){
        data[addr_pointer] = in;
        instr_pointer++;
    }
    void inp_l_start(){
        if (data[addr_pointer] == 0){
            int open_count = 0;
            while (true){
                instr_pointer++;
                if (instr[instr_pointer] == '[') open_count++;
                if (instr[instr_pointer] == ']')
                    if (open_count==0)
                        break;
                    else
                        open_count--;
            }
        }
        instr_pointer++;
    }
    void inp_l_end(){
        if (data[addr_pointer] != 0){
            int open_count = 0;
            while (true){
                instr_pointer--;
                if (instr[instr_pointer] == ']') open_count++;
                if (instr[instr_pointer] == '[')
                    if (open_count==0)
                        break;
                    else
                        open_count--;
            }
        }
        instr_pointer++;
    }

};

template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream << "0x"
         << std::setfill ('0') << std::setw(sizeof(T)*2)
         << std::hex << +i;
  return stream.str();
}


class Tape {
    int xpos, ypos, width, height;
    olc::vi2d data_size, addr_size;
    int data_start_offset, addr_start_offset;
    olc::PixelGameEngine* pge;
public:
    Tape(olc::PixelGameEngine* pge, int xpos, int ypos, int width, int height) :
    pge(pge), xpos(xpos), ypos(ypos), width(width), height(height) {
        data_size = pge->GetTextSizeProp("0x00");
        addr_size = pge->GetTextSizeProp("0x0000");
        data_start_offset = (width - 2 - data_size.x)/2;
        addr_start_offset = (width - 2 - addr_size.x)/2;
    }

    void draw(BF* bf){
        int cell_space = addr_size.y + 2;
        int y_next = ypos + cell_space + 1;

        int center_index = pge->ScreenHeight() / (2*cell_space);

        pge->DrawRect({xpos, ypos}, {xpos+2*width, ypos+height-1});
        pge->DrawLine({xpos+width, ypos}, {xpos+width, ypos+height-1});

        int index = 0;
        int data_x = xpos + width + 1;
        uint16_t addr;
        uint8_t data;
        addr = bf->addr_pointer + center_index - index;
        bf->get_data_at_addr(addr, &data);

        while(y_next < pge->ScreenHeight()){
            pge->DrawLine({xpos,y_next+1}, {xpos+width-1,y_next+1});
            pge->DrawLine({data_x,y_next+1}, {data_x+width-1,y_next+1});

            if (index != center_index){
                pge->DrawString({xpos+addr_start_offset, y_next-cell_space+2}, int_to_hex(addr), olc::WHITE, 1.5);
                pge->DrawString({data_x+data_start_offset, y_next-cell_space+2}, int_to_hex(data), olc::WHITE, 1.5);
            }
            else{
                pge->FillRect({xpos, y_next-cell_space+1}, {xpos+2*width, cell_space}, olc::WHITE);
                pge->DrawString({xpos+addr_start_offset+1, y_next-cell_space+2}, int_to_hex(addr), olc::BLACK, 1.5);
                pge->DrawString({data_x+data_start_offset+1, y_next-cell_space+2}, int_to_hex(data), olc::BLACK, 1.5);
            }
            y_next += cell_space;
            index += 1;
            addr = bf->addr_pointer + center_index - index;
            bf->get_data_at_addr(addr, &data);

        }
        pge->DrawString({xpos+addr_start_offset+1, y_next-cell_space+2}, int_to_hex(addr), olc::WHITE, 1.5);
        pge->DrawString({data_x+data_start_offset+1, y_next-cell_space+2}, int_to_hex(data), olc::WHITE, 1.5);


        while(y_next < pge->ScreenHeight()){

            y_next += cell_space;
        }
    }
};

class BFDisp : public olc::PixelGameEngine
{
public:
    BFDisp()
    {
        sAppName = "BF";
    }

public:
    Tape* tape;
    BF* bf;

    bool OnUserCreate() override
    {
        bf = new BF();
        tape = new Tape(this, 0, 0, 60, ScreenHeight());
        bf->load_program("++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.");

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::BLACK);

        bf->step();


        DrawString(300, 10, std::string(1, bf->instr[bf->instr_pointer]));
        DrawString(310, 10, std::to_string(bf->instr_pointer));
        tape->draw(bf);
        return true;
    }
};


int main()
{
    BFDisp bf;
    if (bf.Construct(512, 480, 4, 4))
    	bf.Start();
    return 0;
}
