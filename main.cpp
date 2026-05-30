#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#define SAMPLE_RATE 44100
#define PI 3.1415926535897932384626433832795028841971

// Types
typedef std::string str_t;
typedef std::vector<double> wave_t;
typedef std::vector<str_t> score_t;
typedef std::function<double(double,double,int)> func_t;
typedef std::map<str_t,func_t> osc_t;

struct note{
    int num;
    double len;
    double vel;
};

typedef std::vector<note> notes_t;

struct fullScore{
    int tempo;
    std::map<str_t,notes_t> parts;
};

// Debug
void print(str_t s){
    std::cout << s << std::endl;
}

void print(double s){
    std::cout << s << std::endl;
}

// notions
class WaveExporter{
    private:
    std::ofstream outFile;
    void writeId(const str_t& id){
        outFile.write(id.c_str(),id.size());
    }
    void writeInt(const int& value){
        outFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
    }
    void writeDouble(const double& value){
        int16_t intValue = static_cast<int16_t>(value * 32767);
        if(intValue < -32768) intValue = -32768;
        if(intValue > 32767) intValue = 32767;
        outFile.write(reinterpret_cast<const char*>(&intValue), sizeof(int16_t));
    }

    public:
    void writeBin(const wave_t& wave,str_t path){
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
    Wave(){}

    Wave(const Wave& wave2){
        add(wave2);
    }

    void add(const Wave& wave2){
        for(auto x : wave2.wave){
            wave.push_back(x);
            if(peak<abs(x)) peak = abs(x);
        }
    }
    void mix(const Wave& wave2){
        if(wave.size()<wave2.wave.size()){
            wave.resize(wave2.wave.size(),0.0);
        }

        for(int i=0;i<wave2.wave.size();i++){
            wave[i] += wave2.wave[i];
            double value = abs(wave[i]);
            if(peak<value) peak = value;
        }
    }
    void volume(const double vol){
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

// instruments
class Oscillator{
    private:
    str_t type;
    osc_t calcSample;

    void calcSample_load(){
        calcSample["SINE"] = [](const double& lambda, const double& freq, const int& i){
            double t = double(i)/SAMPLE_RATE;
            return (double)std::sin(2.0*PI * freq * t);
        };
        calcSample["SQUARE"] = [](const double& lambda, const double& freq, const int& i){
            if(std::fmod(i,lambda) < (lambda/2)) return 1;
            else return -1;
        };
        calcSample["TRIANGLE"] = [](const double& lambda, const double& freq, const int& i){
            if(std::fmod(i,lambda) < (lambda/2)){
                return (std::fmod(i,lambda)/(lambda/2))*2-1;
            }
            else return 1-((std::fmod(i,lambda)/(lambda/2)-1)*2);
        };
        calcSample["SAW"] = [](const double& lambda, const double& freq, const int& i){
            return (std::fmod(i,lambda)/(lambda))*2-1;
        };
        calcSample["EMPTY"] = [](const double& lambda, const double& freq, const int& i){
            return 0;
        };
    }
    
    Wave createWave(const double& freq,const int& duration,const double& vel){
        Wave wave;
        wave.wave.resize(duration);
        int length = round(duration * 1.0);
        double lambda = SAMPLE_RATE/freq;
        auto& func = calcSample[type];

        for(int i=0;i<length;i++){
            wave.wave[i] = func(lambda,freq,i);
            wave.wave[i] *= (vel/100);
        }

        return wave;
    }

    public:
    Oscillator(str_t new_type="SINE"){
        type = new_type;
        calcSample_load();
    }

    void typeChange(const str_t& new_type){
        type = new_type;
    }

    Wave oscillator(const double& freq, const int& duration,const double vel = 72){
        return createWave(freq,duration,vel);
    }
};

class Amplifier{
    private:
    double adsr_filter(int time,double gain,double attack=0.01,double decay=0.1,double sustain=0.2,double release=0.7){
        double level;
        if (time < attack)
                level = time / attack;
            else if (time < attack + decay){
                double rate = (time - attack)/ decay;
                level = 1.0 - (1.0 - gain) * rate;
            }else if (time < attack + decay+sustain){
                level = gain;
            }else{
                double rate = (time - attack - decay - sustain) / release;
                level = gain * (1-rate);
            }
        if(level < 0) level=0;
        if(level > 1) level=1;
        return level;
    }
    public:
    void adsr(double& x,int count,double gain,double attack=0.01,double decay=0.1,double sustain=0.2,double release=0.7){
        attack *= SAMPLE_RATE;
        decay *= SAMPLE_RATE;
        sustain *= SAMPLE_RATE;
        release *= SAMPLE_RATE;
        double level = adsr_filter(count,gain,attack,decay,sustain,release);
        x *= level;
    }
};

class Filter{
    protected:
    double cutoff;
    double last_x;
    
    /*
    public:
    Filter(const double& hz){
        cutoff = hz / SAMPLE_RATE;
        last_x = 0;
    }*/
};

class LPFilter:public Filter{
    public:
    LPFilter(const double& hz){
        cutoff = hz / SAMPLE_RATE;
        last_x = 0;
    }

    double process(double x){
        x = cutoff * x +(1-cutoff) * last_x;
        last_x = x;
        return x;
    }
};

class HPFilter:public Filter{
    public:
    HPFilter(const double& hz){
        cutoff = hz / SAMPLE_RATE;
        last_x = 0;
    }

    double process(double x){
        double lp_out = cutoff * x +(1-cutoff) * last_x;
        x -= lp_out;
        last_x = lp_out;
        return x;
    }
};

class Synth{
    protected:
    Oscillator osc;
    Amplifier amp;

    public:
    /*Synth(const str_t& osc_type="SINE"){
        osc.typeChange(osc_type);
    }*/

    Wave play(const double& freq, const int& duration,const double vel = 72){
        Wave wave = osc.oscillator(freq,duration,vel);
        return wave;
    }
};

class RegularSynth:public Synth{
    public:
    RegularSynth(const str_t& osc_type="SINE"){
        osc.typeChange(osc_type);
    }

    Wave play(const double& freq, const int& duration,const double vel = 72,const double lp_mix = 0.2, const double hp_mix = 0.2){
        Wave wave = osc.oscillator(freq,duration,vel);
        LPFilter lp_filter(freq);
        HPFilter hp_filter(freq);

        for(int i=0;i<wave.wave.size();i++){
            amp.adsr(wave.wave[i],i,0.8);
            wave.wave[i] = (wave.wave[i] * (1-lp_mix)) + (lp_mix * lp_filter.process(wave.wave[i]));
            wave.wave[i] = (wave.wave[i] * (1-hp_mix)) + (hp_mix * hp_filter.process(wave.wave[i]));
        }

        return wave;
    }
};

class HarmonicSynth:public Synth{
    public:
    HarmonicSynth(const str_t& osc_type="SINE"){
        osc.typeChange(osc_type);
    }

    Wave play(const double& freq, const int& duration,const double vel = 72,const double overtone_limit = 16){
        Wave wave,overtone;
        LPFilter lp_filter(freq);
        double lp_mix=0.3;

        for(int i=1;i<=overtone_limit;i++){
            overtone = osc.oscillator(i*freq,duration,vel);
            double gain = (1.0/(2+i^2)) / 2.0;
            for(int j=0;j<overtone.wave.size();j++){
                overtone.wave[j] *= gain;
            }
            wave.mix(overtone);
        }

        for(int i=0;i<wave.wave.size();i++){
            amp.adsr(wave.wave[i],i,0.8);
            wave.wave[i] = (wave.wave[i] * (1-lp_mix)) + (lp_mix * lp_filter.process(wave.wave[i]));
        }
        
        return wave;
    }
};


// effects
void compressor(Wave& wave){
    printf("%lf",wave.getPeak());
    for(auto& sample : wave.wave){
        sample /= wave.getPeak();
    }
}

// Players
class Musician{ //knowledge
    private:
    std::vector<str_t> notes = {
            "C","Des","D","Es",
            "E","F","Ges","G",
            "As","A","B","H"
    };
    protected:
    double concert_pitch = 440;

    int pcset(const str_t& note){
        for(int i=0;i<12;i++)
            if(note == notes[i]) return i;

        print("error: pcset not found");
        return 0;
    }

    str_t num2note(const int& note_num){
        int pcset = note_num%12;
        return notes[pcset];
    }

    int note_num(const int& pcset,const int octave=4){
        return pcset + 12*octave;
    }
};

class Composer:public Musician{
    private:
    void makeMelody(fullScore& kirakira){
        int i,j,bin=0b0110;
        score_t melody1 = {
            "C","C","G","G","A","A","G", //ドドソソララソ
            "F","F","E","E","D","D","C" //ファファミミレレド
        };
        score_t melody2 = {
            "G","G","F","F","E","E","D"
        };

        for(i=0;i<4;i++){
            if((bin>>i) & 1){
                j=1;
                for(auto x: melody2){
                    kirakira.parts["melody"].push_back({
                        note_num(pcset(x)),
                        j % 7 ? 4.0 : 2.0,
                        72
                    });
                    j++;
                }
            }
            else {
                j=1;
                for(auto x: melody1){
                    kirakira.parts["melody"].push_back({
                        note_num(pcset(x)),
                        j % 7 ? 4.0 : 2.0,
                        72
                    });
                    j++;
                }
            };
        }
    }

    bool isCmaj(const int& note){
        int pc = note%12;
        std::vector<int> diatonic = {0,2,4,5,7,9,11};
        for(auto x : diatonic){
            if(pc == x) return true;
        }
        return false;
    }

    std::vector<int> vib3(const int& a,const int& b ,const int& c){
        return {a,b,c,b,c,b,c,b};
    }

    void Sub_type1(fullScore& kirakira,const str_t& a,const int& an,const str_t& b,const int& bn,const str_t& c,const int& cn){
        std::vector<int> sub8 = vib3(
            note_num(pcset(a),an),
            note_num(pcset(b),bn),
            note_num(pcset(c),cn)
        );
        for(int i=0;i<sub8.size();i++)
            kirakira.parts["sub"].push_back({
            sub8[i],
            16,
            72
        });
    }

    void Sub_type2(fullScore& kirakira,const str_t& a,const int& an,const str_t& b,const int& bn){
        int count=0;
        int note;
        note = note_num(pcset(a),an);
        while(count<8){
            if(count==2)note = note_num(pcset(b),bn);
            if(isCmaj(note)){
                kirakira.parts["sub"].push_back({
                    note,
                    16,
                    72
                });
                count++;
            }
            note--;
        }
    }

    void Sub_manual(fullScore& kirakira,
        const str_t& a,const int& an,const str_t& b,const int& bn,const str_t& c,const int& cn,const str_t& d,const int& dn,
        const str_t& e,const int& en,const str_t& f,const int& fn,const str_t& g,const int& gn,const str_t& h,const int& hn
    ){
        std::vector<str_t> sub8_note = {a,b,c,d,e,f,g,h};
        std::vector<int> sub8_oct = {an,bn,cn,dn,en,fn,gn,hn};
        for(int i=0;i<8;i++)
            kirakira.parts["sub"].push_back({
            note_num(pcset(sub8_note[i]),sub8_oct[i]),
            16,
            72
        });
    }

    void makeSub(fullScore& kirakira){
        int i,j,bin=0b0110;
        for(i=0;i<4;i++){
            if((bin>>i) & 1){
                Sub_type1(kirakira,"E",5,"C",5,"H",4);
                Sub_type1(kirakira,"D",5,"H",4,"B",4);
                Sub_type1(kirakira,"C",5,"A",4,"As",4);
                Sub_type1(kirakira,"H",4,"G",4,"Ges",4);
            }
            else {
                Sub_type1(kirakira,"E",5,"C",5,"H",4);
                Sub_type1(kirakira,"A",5,"G",5,"Ges",5);
                Sub_manual(kirakira,
                    "As",5,"A",5,"C",6,"H",5,
                    "D",6,"C",6,"H",5,"A",5
                );
                
                Sub_type2(kirakira,"A",5,"E",6);
                Sub_type2(kirakira,"G",5,"D",6);
                Sub_type2(kirakira,"F",5,"C",6);

                Sub_manual(kirakira,
                    "D",5,"D",5,"A",5,"A",5,
                    "G",5,"G",5,"H",4,"H",4
                );
                kirakira.parts["sub"].push_back({
                    note_num(pcset("C"),5),
                    2,
                    72
                });
            }
        }
    }

    void makeBass(fullScore& kirakira){
        int i,j,bin=0b0110;
        score_t bass1 = {
            "C","E","F","E",
            "D","C","H","C"
        };
        score_t bass2 = {
            "E","D","C","H"
        };

        for(i=0;i<4;i++){
            if((bin>>i) & 1){
                j=1;
                for(auto x: bass2){
                    kirakira.parts["bass"].push_back({
                        note_num(pcset(x),j % 4 ? 4 : 3),
                        4,
                        72
                    });
                    kirakira.parts["bass"].push_back({
                        43,
                        4,
                        72
                    });
                    j++;
                }
            }
            else {
                j=0;
                for(auto x: bass1){
                    kirakira.parts["bass"].push_back({
                        note_num(pcset(x),j%6?4:3),
                        4,
                        72
                    });
                    kirakira.parts["bass"].push_back({
                        43,
                        4,
                        72
                    });
                    j++;
                }
            }
        }
    }
    public:
    fullScore createkirakira(){
        fullScore kirakira;
        kirakira.tempo = 132;
        
        makeMelody(kirakira);
        makeSub(kirakira);
        makeBass(kirakira);
        return kirakira;
    }
};

class Player:public Musician{
    private:
    str_t part;
    str_t inst_type;
    str_t osc_type;
    double a;
    double b;
    public:
    Player(const str_t& new_part){
        part = new_part;
    }
    void get_instrument(const str_t& _osc_type, const str_t& _inst_type, double _a,double _b=0){
        osc_type = _osc_type;
        inst_type = _inst_type;
        a = _a;
        b = _b;
    }
    Wave play(fullScore& kirakira){
        Wave wave;
        RegularSynth r_inst(osc_type);
        HarmonicSynth h_inst(osc_type);

        double semitone = std::pow(2,1.0/12.0);
        double freq;

        int tempo = 132;
        double time;

        for(int i=0;i<kirakira.parts[part].size();i++){
            note& note = kirakira.parts[part][i];
            freq = concert_pitch * std::pow(semitone,note.num - 57 );
            time = 240 / (tempo * note.len);
            int duration = static_cast<int>(time * SAMPLE_RATE);
        
            if(inst_type == "REGULAR")
                wave.add(r_inst.play(freq,duration,note.vel,a,b));
            else if(inst_type == "HARMONIC")
                wave.add(h_inst.play(freq,duration,note.vel,a));
        }
        return wave;
    }
};

int main(void){
    const str_t path = "./01.wav";
    // create music
    Composer Chopin;
    fullScore kirakira = Chopin.createkirakira();

    // play
    Player Sakana("melody");
    Player Neko("sub");
    Player Kurage("bass");

    Sakana.get_instrument("SQUARE","HARMONIC",8);
    Neko.get_instrument("SINE","REGULAR",0.2,0.8);
    Kurage.get_instrument("TRIANGLE","REGULAR",0.6,0.1);

    Wave sound_s(Sakana.play(kirakira));
    Wave sound_n(Neko.play(kirakira));
    Wave sound_k(Kurage.play(kirakira));

    sound_k.volume(2);

    // mix & mastering
    Wave master;
    master.mix(sound_s);
    master.mix(sound_n);
    master.mix(sound_k);
    print(sound_s.getPeak());
    print(sound_n.getPeak());
    print(sound_k.getPeak());

    compressor(master);
    master.volume(0.4);
    master.exportFile(path);   
    
    return 0;
}