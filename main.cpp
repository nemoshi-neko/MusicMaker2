#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#define SAMPLE_RATE 44100
#define PI 3.1415926535897932384626433832795028841971

typedef std::string str_t;
typedef std::vector<double> wave_t;

void print(str_t s){
    std::cout << s << std::endl;
}

wave_t CreateWave(int freq, int time){
    int duration = time * SAMPLE_RATE;
    wave_t wave(duration);

    for(int i=0;i<duration;i++){
        double t = double(i)/SAMPLE_RATE;
        wave[i] = (double)std::sin(2.0*PI * freq * t);
    }
    return wave;
}

class WriteWave{
    private:
    std::ofstream outFile;
    void WriteId(str_t id){
        outFile.write(id.c_str(),id.size());
    }
    void WriteInt(int value){
        outFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
    }
    void WriteDouble(double value){
        int16_t intValue = static_cast<int16_t>(value * 32767);
        if(intValue < -32768) intValue = -32768;
        if(intValue > 32767) intValue = 32767;
        outFile.write(reinterpret_cast<const char*>(&intValue), sizeof(int16_t));
    }

    public:
    void WriteBin(wave_t wave,str_t path){
        outFile.open(path, std::ios::out | std::ios::binary);
        if(!outFile){print("error: WriteBin -> outFile");return;}

        int dataSize = wave.size()*2;

        WriteId("RIFF");
        WriteInt(dataSize +36);
        WriteId("WAVE");

        WriteId("fmt ");
        WriteInt(0x10);
        WriteInt(0x00010001);
        WriteInt(0x0000ac44);
        WriteInt(0x00015888);
        WriteInt(0x00100002);

        WriteId("data");
        WriteInt(dataSize);
        for(auto x : wave){
            WriteDouble(x);
        }
    }
};

int main(void){
    const str_t path = "./01.wav";
    wave_t wave(SAMPLE_RATE, 0.0);
    wave = CreateWave(440, 1);
    WriteWave track1;
    track1.WriteBin(wave,path);
    return 0;
}
