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

struct fullScore{
    score_t melody;
    score_t back;
};

void print(str_t s){
    std::cout << s << std::endl;
}

void print(double s){
    std::cout << s << std::endl;
}

void compressor(wave_t& wave,double peak){
    printf("%lf",peak);
    for(auto& sample : wave){
        sample /= peak;
    }
}

fullScore createkirakira(){
    double semitone = std::pow(2,1.0/12.0);
    double a = 440;
    double b = a * std::pow(semitone,2);
    double c = a * std::pow(semitone,3);
    double d = a * std::pow(semitone,5);
    double e = a * std::pow(semitone,7);
    double f = a * std::pow(semitone,8);
    double g = a * std::pow(semitone,10);

    
    score_t melody1 = {
        c,c,g,g,2*a,2*a,g, //ドドソソララソ
        f,f,e,e,d,d,c //ファファミミレレド
    };
    score_t melody2 = {
        g,g,f,f,e,e,d
    };
    score_t back1 = {
        c/4,e/2,f/2,e/2,
        d/2,c/2,b/2,c/2
    };
    score_t back2 = {
        e/2,d/2,c/2,b/2
    };
    
    fullScore kirakira;
    int i,bin=6; // 6 = 0b0110
    for(i=0;i<4;i++){
        if((bin>>i) & 1)for(auto x: melody2)kirakira.melody.push_back(x);
        else for(auto x: melody1)kirakira.melody.push_back(x);
    }
    for(i=0;i<4;i++){
        if((bin>>i) & 1){
            for(auto x: back2){
                kirakira.back.push_back(x);
                kirakira.back.push_back(g/4);
            }
        }
        else {
            for(auto x: back1){
                kirakira.back.push_back(x);
                kirakira.back.push_back(g/4);
            }
        }
    }
    return kirakira;
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
            if(peak<x) peak = abs(x);
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
    str_t type;

    double calcSample(double lambda, double freq, int i){
        if(type == "SINE"){
            double t = double(i)/SAMPLE_RATE;
            return (double)std::sin(2.0*PI * freq * t);
        }else if(type == "SQUARE"){
            if(std::fmod(i,lambda) < (lambda/2)) return 1;
            else return -1;
        }else if(type == "TRIANGLE"){
            if(std::fmod(i,lambda) < (lambda/2)){
                return (std::fmod(i,lambda)/(lambda/2))*2-1;
            }
            else return 1-((std::fmod(i,lambda)/(lambda/2)-1)*2);
        }else if(type == "SAW"){
            return (std::fmod(i,lambda)/(lambda))*2-1;
        }else {
            return 0;
        }
    }
    
    wave_t createWave(double freq,int duration){
        wave_t wave(duration);
        int length = round(duration * 0.9);
        double lambda = SAMPLE_RATE/freq;

        for(int i=0;i<length;i++){
            wave[i] = calcSample(lambda,freq,i);
        }

        return wave;
    }

    public:
    Synth(str_t new_type="SINE"){
        type = new_type;
    }

    void typeChange(str_t new_type){
        type = new_type;
    }

    wave_t oscillator(double freq, double time){
        int duration = static_cast<int>(time * SAMPLE_RATE);
        return createWave(freq,duration);
    }
};

int main(void){
    const str_t path = "./01.wav";
    Wave wave01;
    Wave wave02;
    Synth synth01("SQUARE");
    Synth synth02("TRIANGLE");

    // score & tempo section
    fullScore kirakira = createkirakira();
    
    double mini = 1.0/2.0;
    double crot = 1.0/4.0;

    // synth
    int i=1;
    for(auto x: kirakira.melody){
        if(i % 7) wave01.add(synth01.oscillator(x,crot));
        else wave01.add(synth01.oscillator(x,mini));
        i++;
    }

    for(auto x: kirakira.back){
        wave02.add(synth02.oscillator(x,crot));        
    }
    wave02.volume(1.5);

    //mix & mastering
    wave01.mix(wave02.wave);
    compressor(wave01.wave, wave01.getPeak());
    wave01.volume(0.4);

    wave01.exportFile(path);
    return 0;
}