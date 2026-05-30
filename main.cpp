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
typedef std::vector<double> score_t;

void print(str_t s){
    std::cout << s << std::endl;
}

void compressor(wave_t& wave,double peak){
    printf("%lf",peak);
    for(auto& sample : wave){
        sample /= peak;
    }
}

class WaveExporter{
    private:
    std::ofstream outFile;
    void writeId(str_t id){
        outFile.write(id.c_str(),id.size());
    }
    void writeInt(int value){
        outFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
    }
    void writeDouble(double value){
        int16_t intValue = static_cast<int16_t>(value * 32767);
        if(intValue < -32768) intValue = -32768;
        if(intValue > 32767) intValue = 32767;
        outFile.write(reinterpret_cast<const char*>(&intValue), sizeof(int16_t));
    }

    public:
    void writeBin(wave_t wave,str_t path){
        outFile.open(path, std::ios::out | std::ios::binary);
        if(!outFile){print("error: WriteBin -> outFile");return;}

        int dataSize = wave.size()*2;

        writeId("RIFF");
        writeInt(dataSize +36);
        writeId("WAVE");

        writeId("fmt ");
        writeInt(0x10);
        writeInt(0x00010001);
        writeInt(0x0000ac44);
        writeInt(0x00015888);
        writeInt(0x00100002);

        writeId("data");
        writeInt(dataSize);
        for(auto x : wave){
            writeDouble(x);
        }
    }
};

class Wave{
    private:
    double peak = 0.0;
    public:
    wave_t wave;
    void add(wave_t wave2){
        for(auto x : wave2){
            wave.push_back(x);
            if(peak<x) peak = x;
        }
    }
    void mix(wave_t wave2){
        int minSize = std::min(wave.size(), wave2.size());
        for(int i=0;i<minSize;i++){
            wave[i] += wave2[i];
            double value = abs(wave[i]);
            if(peak<value) peak = value;
        }
    }
    void volume(double vol){
        for(auto& x : wave){
            x *= vol;
        }
    }
    double getPeak(){
        return peak;
    }
    void exportFile(str_t path){
        WaveExporter exporter01;
        exporter01.writeBin(wave, path);
    }
};

class Synth{
    private:
    wave_t createSinWave(double freq, int duration){
        wave_t wave(duration);

        for(int i=0;i<duration;i++){
            double t = double(i)/SAMPLE_RATE;
            wave[i] = (double)std::sin(2.0*PI * freq * t);
        }
        return wave;
    }

    wave_t createSquareWave(double freq, int duration){
        wave_t wave(duration);
        double lambda = SAMPLE_RATE/freq;
        for(int i=0;i<duration;i++){
            if(std::fmod(i,lambda) < (lambda/2)) wave[i] = 1;
            else wave[i] = -1;
        }
        return wave;
    }

    public:
    wave_t oscillator(str_t type, double freq, double time){
        int duration = static_cast<int>(time * SAMPLE_RATE);
        if(type == "SINE") return createSinWave(freq,duration);
        else if(type == "SQUARE") return createSquareWave(freq,duration);
        return createSinWave(freq,duration);
    }
};

int main(void){
    const str_t path = "./01.wav";
    Wave wave01;
    Wave wave02;
    Synth synth01;

    double semitone = std::pow(2,1.0/12.0);
    double a = 440;
    double b = a * std::pow(semitone,2);
    double c = a * std::pow(semitone,3);
    double d = a * std::pow(semitone,5);
    double e = a * std::pow(semitone,7);
    double f = a * std::pow(semitone,8);
    double g = a * std::pow(semitone,10);

    double mini = 1.0/2.0;
    double crot = 1.0/4.0;
    score_t melody = {
        c,c,g,g,2*a,2*a,g, //ドドソソララソ
        f,f,e,e,d,d,c //ファファミミレレド
    };
    score_t back = {
        c/4,e/4,f/4,e/4,
        d/4,c/4,b/4,c/4
    };

    int i=1;
    for(auto x: melody){
        if(i % 7) wave01.add(synth01.oscillator("SINE",x,crot));
        else wave01.add(synth01.oscillator("SINE",x,mini));
        i++;
    }

    for(auto x: back){
        wave02.add(synth01.oscillator("SQUARE",x,mini));
    }
    wave02.volume(0.5);

    //mix & mastering
    wave01.mix(wave02.wave);
    compressor(wave01.wave, wave01.getPeak());
    wave01.volume(0.4);

    wave01.exportFile(path);
    return 0;
}
