
const String clockStringFrench =  "IL EST DEUXQUATRETROISNEUFUNESEPTHUITSIXCINQMIDI MINUITONZE HEURESMOINS ETDIXETQUART    VINGT-CINQ ET DEMIE   TRENTE-CINQ";

/**
 * @brief Draw the given sentence to the word clock
 * 
 * @param message sentence to be displayed
 * @param color 24bit color value
 * @return int: 0 if successful, -1 if sentence not possible to display
 */
int showStringOnClock_french(String message, uint32_t color){
    int messageStart = 0;
    String word = "";
    int lastLetterClock = 0;
    int positionOfWord  = 0;
    int nextSpace = 0;
    int index = 0;

    // add space on the end of message for splitting
    message = message + " ";

    // empty the targetgrid
    ledmatrix.gridFlush();

    while(true){
      // extract next word from message
      word = split(message, ' ', index);
      index++;
      
      if(word.length() > 0){
        // find word in clock string
        positionOfWord = clockStringFrench.indexOf(word, lastLetterClock);
        
        if(positionOfWord >= 0){
          // word found on clock -> enable leds in targetgrid
          for(int i = 0; i < word.length(); i++){
            int x = (positionOfWord + i)%WIDTH;
            int y = (positionOfWord + i)/WIDTH;
            ledmatrix.gridAddPixel(x, y, color);
          }
          // remember end of the word on clock
          lastLetterClock = positionOfWord + word.length();
        }
        else{
          // word is not possible to show on clock
          logger.logString("word is not possible to show on clock: " + String(word));
          return -1;
        }
        //logger.logString(String(nextSpace) + " - " + String());
      }else{
        // end - no more word in message
        break;
      }
    }
    // return success
    return 0;
}

/**
 * @brief convert the given number to french
 * 
 * @param number number to be converted
 * @return String number as french
 */
String numberToFrench(uint8_t number) {
  switch (number) {
    case 1: return "UNE";
    case 2: return "DEUX";
    case 3: return "TROIS";
    case 4: return "QUATRE";
    case 5: return "CINQ";
    case 6: return "SIX";
    case 7: return "SEPT";
    case 8: return "HUIT";
    case 9: return "NEUF";
    case 10: return "DIX";
    case 11: return "ONZE";
    case 12: return "DOUZE";
    default: return "";
  }
}

/**
 * @brief Converts the given time as sentence (String)
 * 
 * @param hours hours of the time value
 * @param minutes minutes of the time value
 * @return String time as sentence
 */
String timeToString_french(uint8_t hours, uint8_t minutes) {
  // Runden der Minuten auf den nächsten 5-Minuten-Takt
  minutes = minutes / 5 * 5;

  // ES IST
  String message = "IL EST ";

  // Stunden anpassen, falls Minuten >= 60 (da wir dann zur nächsten Stunde gehen)
  if (minutes >= 60) {
    minutes = 0;
    hours = (hours + 1) % 24;
  }
  
  if(minutes >= 35)
  {
      hours++;
  }

  // Umwandlung in 12-Stunden-Format
  if (hours == 0) {
    message += "MINUIT";
  } else if (hours == 12) {
    message += "MIDI";
  } else {
    if (hours > 12) {
      hours -= 12;
    }
    message += numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "");
  }

  // Minuten formatieren
  if (minutes == 0) {
    // Keine Minuten, nur volle Stunde
    return message;
  } else if (minutes == 5) {
    message += " CINQ";
  } else if (minutes == 10) {
    message += " DIX";
  } else if (minutes == 15) {
    message += " ET QUART";
  } else if (minutes == 20) {
    message += " VINGT";
  } else if (minutes == 25) {
    message += " VINGT-CINQ";
  } else if (minutes == 30) {
    message += " ET DEMIE";
  } else if (minutes == 35) {
    message = "IL EST " + numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "") + " MOINS VINGT-CINQ";
  } else if (minutes == 40) {
    message = "IL EST " + numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "") + " MOINS VINGT";
  } else if (minutes == 45) {
    message = "IL EST " + numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "") + " MOINS LE QUART";
  } else if (minutes == 50) {
    message = "IL EST " + numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "") + " MOINS DIX";
  } else if (minutes == 55) {
    message = "IL EST " + numberToFrench(hours) + " HEURE" + (hours > 1 ? "S" : "") + " MOINS CINQ";
  }

  logger.logString("time as String: " + String(message));
  return message;
}

