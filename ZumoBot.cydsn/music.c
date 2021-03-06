/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/* [] END OF FILE */

#include "Systick.h"
#include "Beep.h"
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include "music.h"
#define FREQ_A 440.0f
#define BASE_OCTAVE (-3)
#define MAX_PERIOD (327.68f / 1000)
#define PERIOD_PWM (65536)

void stop_music(){
    Buzzer_PWM_Stop();
}

void PlayBufferPDM(uint16* buffer, uint8 size)
{
    SPIM_PDM_PutArray(buffer, size);
}

void PlayPDM(uint16* track, int size)
{
    SPIM_PDM_Enable();
    SPIM_PDM_Start();
    
    const uint8 BUFFER_SIZE = 255;
    uint16 buffer[BUFFER_SIZE];
    
    //Load the music data into chuck of size BUFFER_SIZE and play them
    for (int i = 0; i < size; i++){
        buffer[i%BUFFER_SIZE] = track[i];
        if (i % BUFFER_SIZE == BUFFER_SIZE - 1){
          PlayBufferPDM(buffer, BUFFER_SIZE);   
        }
    }
    
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0;
    
    PlayBufferPDM(buffer, BUFFER_SIZE);
    
    CyDelay(100);
    
    SPIM_PDM_Stop();
}

//Start playing a note asyncronously
void Beep16AsyncStart(uint16 pitch)
{
    uint16 cmp = pitch / 2;
    Buzzer_PWM_Start();
    Buzzer_PWM_WriteCompare(cmp);
    Buzzer_PWM_WritePeriod(pitch);
}

//Stop playing an async note
void Beep16AsyncStop()
{
    Buzzer_PWM_Stop();
}

//Play note without legato
void Beep16(uint32 length, uint16 pitch)
{
    uint16 cmp = pitch / 2;
    Buzzer_PWM_Start();
    Buzzer_PWM_WriteCompare(cmp);
    Buzzer_PWM_WritePeriod(pitch);
    CyDelay(length);
    Buzzer_PWM_Stop();
}

//Play note with legato
void Beep16L(uint32 length, uint16 pitch)
{
    uint16 cmp = pitch / 2;
    Buzzer_PWM_Start();
    Buzzer_PWM_WriteCompare(cmp);
    Buzzer_PWM_WritePeriod(pitch);
    CyDelay(length);
}

//Parse a note from a character
//If the parsing cannot be concluded because we need an additional characted, return false
//If the parser is concluded, return true
bool parse_note(Note* note, char cnote, int base_duration){
    char c = cnote;
    
    if (isdigit(c)){
        //Convert the character to int and set the octave
        note->octave = c - '0';
    } else if (c == 'O') {
        note->duration = base_duration * 4;
    } else if (c == 'o') {
        note->duration = base_duration * 2;
    } else if (c == '.') {
        note->duration = (note->duration) * 1.5;
    } else if (c == '-') {
        note->duration = base_duration / 2;
    } else if (c == '=') {
        note->duration = base_duration / 4;
    } else if (c == 'L') {
        note->legato = true;
    } else if (c == ' ') {
        note->duration = base_duration;
        note->legato = false;
    } else {
        note->note = c;
        return true;
        //play_note(c, base, *octave, *length);   
    }
    return false;
}

//Experimental
//Play two music tracks at the same time by alternating quickly between two notes
void play_music_with_base(char* music, char* baseline, float base_duration){
    //The idea is to split every note for both main and base
    //tracks into elementary units of length 1/16 of a quarter note.
    //Each unit will then be played according to this rule:
    //If we have units from the main track alone, play them
    //If we have units from the base track alone, play them
    //If we have units from both of them, play first the main and then the base
    
    
    const float unit_duration = base_duration / 16;
    int i = 0, j = 0;
    bool endmusic = false;
    char c = music[0];
    char b = baseline[0];
    int n_c = 0, n_b = 0; //Number of elementary units left to play, for the main track (n_c) and the base (n_b)
    Note note, base;
    Note tmp_note, tmp_base;
    
    note.octave = 0;
    note.duration = base_duration;
    note.legato = false;
    
    base.note = '\0';
    base.octave = 0;
    base.duration = base_duration;
    base.legato = false;
    
    while (!endmusic){
        
       //If we successfully parsed a note from the main track
        if (c != '\0' && n_c == 0 && parse_note(&note, c, base_duration)){
            //Compute how many units it lasts
            n_c = ((note.duration / unit_duration)/2);
            //Override duration
            tmp_note.octave = note.octave;
            tmp_note.duration = unit_duration;
            tmp_note.legato = false;
            tmp_note.note = note.note;
        }
        
        //If we successfully parsed a note from the base track
        if (b != '\0' && n_b == 0 && parse_note(&base, b, base_duration)){
            //Compute how many units it lasts
            n_b = ((base.duration / unit_duration)/2);
            //Override duration
            tmp_base.octave = base.octave;
            tmp_base.duration = unit_duration;
            tmp_base.legato = false;
            tmp_base.note = base.note;
        }
            
        //There are units from both the main and the base track to play
        while (n_b != 0 && n_c != 0){
            tmp_note.legato = true;
            play_note(tmp_note);
            tmp_base.legato = true;
            play_note(tmp_base);
            n_b--; n_c--;
        }
        
        //There are only units from the base track to play
        while (c == '\0' && n_c == 0 && n_b != 0){
            tmp_base.legato = true;
            play_note(tmp_base);   
            n_b--;
        }
        
        //There are only units from the main track to play
        while (b == '\0' && n_b == 0 && n_c != 0){
            tmp_note.legato = true;
            play_note(tmp_note); 
            n_c--;
        }
        
        //If we played all the units, fetch the next note from the main track
        if (c != '\0' && n_c == 0){
            i++;
            c = music[i];
        }
        
        //If we played all the units, fetch the next note from the base track
        if (b != '\0' && n_b == 0){
            j++;
            b = baseline[j];
        }
        
        //The music ended if this condition is true
        endmusic = c == '\0' && b == '\0' && n_c < 1 && n_b < 1;

    }
    
    stop_music();
    
}

void play_music(char* music, float base_duration){
    int i = 0;
    char c = music[0];
    Note note;
    
    note.legato = false;
    note.octave = 0;
    note.duration = base_duration;
    
    while (c != '\0'){
        //If we successfully parsed a note
        if (parse_note(&note, c, base_duration)){
            //Play it
            play_note(note);
        }
        c = music[i];
        i++;
    }
    
    stop_music();
    
}


//State for the async music playing
Note music_async_current_note;
bool music_async_stopped;
bool music_async_note_playing;
int music_async_note_last_time;
int music_async_index;
char* music_async_data;
float music_async_base_duration;

void set_music_async(char* music, float base_duration){
    music_async_current_note.octave = 0;
    music_async_current_note.legato = false;
    music_async_note_playing = false;
    music_async_note_last_time = 0;
    music_async_index = 0;
    music_async_data = music;
    music_async_base_duration = base_duration;
    music_async_stopped = false;
}

void stop_music_async(){
    music_async_stopped = true;
    Beep16AsyncStop();
}

void play_music_async(){
    
    char c = music_async_data[music_async_index];
    
    //Loop back to the beginning if we reached the end of the track
    if (c == '\0'){
        music_async_index = 0;
        c = music_async_data[music_async_index];
    }
    
    //The track has been stopped
    if (music_async_stopped){
        stop_music();
        return;
    }
    
    //If we are playing a note
    if (music_async_note_playing){
      int time = GetTicks();
    
      //Check if we should stop playing the note
      if (music_async_note_last_time + music_async_current_note.duration  < time){
        Beep16AsyncStop();  
        music_async_note_playing = false;
        return;
      } else {
        return;
      }
    }
    
    
    if (c != '\0'){
        //If we successfully parsed a note
        if (parse_note(&music_async_current_note, c, music_async_base_duration)){
            //Play it
            play_note_async(music_async_current_note);
            music_async_note_last_time = GetTicks();
            music_async_note_playing = true;
        }

        music_async_index++;
    }
    
    
    
}

int note_index(char note){
        int n = 0;
    
    switch(note){
      case 'a':
        n = -1;
        break;
      case 'A':
        n = 0;
        break;
      case 'b':
        n = 1;
        break;
      case 'B':
        n = 2;
        break;
      case 'C':
        n = 3;
        break;
      case 'd':
        n = 4;
        break;
      case 'D':
        n = 5;
        break;
      case 'e':
        n = 6;
        break;
      case 'E':
        n = 7;
        break;
      case 'F':
        n = 8;
        break;
      case 'g':
        n = 9;
        break;
      case 'G':
        n = 10;
        break;     
        
    }
    
    return n;
}

void play_note(Note note){
    int n = note_index(note.note);
    
    if (note.note == 'S'){
        //Silence
        CyDelay(note.duration);
        return;
    }
    
    
    const float freq = FREQ_A * powf(2, BASE_OCTAVE + note.octave + (float)n / 12);
    
    const float period = 1 / freq;
    const float unit_period = MAX_PERIOD / PERIOD_PWM;
    
    const float pwm = period / unit_period;
    
    
    if (pwm < 1.0f || pwm >= PERIOD_PWM){
     printf("Can not play this note. \n");   
    }
    
    if (note.legato)
        Beep16L(note.duration, (uint16) pwm);
    else
        Beep16(note.duration, (uint16) pwm);

}



void play_note_async(Note note){
    int n = note_index(note.note);
    
    if (note.note == 'S'){
        //Silence
        Beep16AsyncStart(0);
        Beep16AsyncStop();
        return;
    }
    
    const float freq = FREQ_A * powf(2, BASE_OCTAVE + note.octave + (float)n / 12);
    
    const float period = 1 / freq;
    const float unit_period = MAX_PERIOD / PERIOD_PWM;
    
    const float pwm = period / unit_period;
    
    
    if (pwm < 1.0f || pwm >= PERIOD_PWM){
     printf("Can not play this note. \n");   
    }
    
    Beep16AsyncStart((uint16) pwm);
    

}
