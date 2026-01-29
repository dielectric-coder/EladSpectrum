INSTALLATION AND USAGE
----------------------
Some notes to install elad-gqrx

* elad-gqrx launch elad-firmware from the invocation directory 
  so cd to that directory before launch as:
  cd your-installation-dir; ./elad-gqrx

* elad-gqrx must be launched as root or using sudo if you do not
  set up the usb interfaces to be used by othe user(s)

* elad-gqrx launches every time elad-firmware in order to
  set the S1/S2 fpga, then it launches gqrx

* elad-gqrx rewrite gqrx configuration file into
  ~/.config/gqrx/default.conf 
  so, if it has important setup for you, save a copy before
  (this can be done using gqrx itself)

* elad-gqrx is only a "wrapper" for gqrx, you must have a copy
  of gqrx installed

* elad-gqrx launches gqrx with no path, so gqrx should be in
  a folder in PATH environment.
  Nevertheless, if you have gqrx in a "strange" folder you can 
  set an environment variable:
  export GQRX_PATH=/your-strange-dir/
  and elad-gqrx starts gqrs as /your-strange-dir/gqrx

* elad-gqrx act silently: to see operations, or to debug
  set the environment variable GQRX_DEBUG before
  running elad-gqrx using
  export GQRX_DEBUG=1

* also elad-firmware acts silantly: to see operations, or to debug
  set the environment variable ELAD_DEBUG before
  running elad-gqrx using
  export ELAD_DEBUG=1

* without parameters elad-gqrx starts with 192kS/s, LPF on, ATT off,
  freq 14.200 kHz, but these values can be changed at launch time using:
  ./elad-gqrx serial startFreq LPATT rate

  where:

  serial: is the device serial number or + (plus) to accept all.
  startFreq: is the start frequency.
  LPATT: 00 o 01 o 10 o 11
         first digit set to 1 enable LowPassFilter
         second digit set to 1 enable Attenuator
  rate: sampling rate
        1 =  192 kS/s
        2 =  384 kS/s
        3 =  768 kS/s
        4 = 1516 kS/s
        5 = 3072 kS/s
        6 = 6144 kS/s

* elad-gqrx uses "gqrx remote control" (tcp port 7356 on localhost)
  to know gqrx hardware frequency and set S1/S2 to this frequency,
  so "remote control" has to be enabled in gqrx.

* MAC users has to enable this connection in their firewall

=====================================================================================
  
* elad-gqrx va lanciato da root o come sudo elad-gqrx
* elad-gqrx lancia sempre elad-firmware
* elad-gqrx sovrascrive il file ~/.config/gqrx/default.conf
* elad-gqrx va lanciato dal suo foldfer, nel quale
  ci deve essere anche elad-firmware
* deve essere installato gqrx.

Parametri

elad-gqrx serial startFreq LPFATT rate
dove:
serial: è il serial dell'apparato o il segno + (plus)
startFreq: è la frequenza a cui parte, in Hz
LPATT: 00 o 01 o 10 o 11
       la prima cifra a 1 attiva il LowPassFilter
       la seconda cifra a 1 attiva l'attenuatore
rate: da 1 a 6 compreso
      1 =  192 kS/s
      2 =  384 kS/s
      3 =  768 kS/s
      4 = 1516 kS/s
      5 = 3072 kS/s
      6 = 6144 kS/s

elad-gqrx fa un sacco di output di debug, se non serve si aggiunga al
comando la stringa:
> /dev/null 2>/dev/null

Alvune note di installazione ed uso di elad-gqrx

* elad-gqrx lancia elad-firmware dalla cartella da cui è lanciato 
  quindi si suggerisce di mettersi sulla cartella di elad-gqrx prima
  di lanciarlo operando come segue:
  cd your-installation-dir; ./elad-gqrx

* elad-gqrx va lanciato come root o usando sudo a mano che non si
  siano abilitate le USB all'uso da parte di altri utenti

* elad-gqrx lancia ogni volta elad-firmware per settare la fpga di
  S1/S2 prima di lanciare gqrx

* elad-gqrx riscrive la configurazione di gqrx presente in
  ~/.config/gqrx/default.conf 
  se in quella configurazione vi sono dati importanti si suggerisce
  di salvarla con un altro nome, usando le opzioni di salvataggio
  della configurazione presenti nel menu di gqrx

* elad-gqrx è solo un "wrapper" attorno a gqrx, per poterlo usare 
  si deve avere gqrx installato

* elad-gqrx lancia gqrx senza path, dato che gqrx dovrebbe essere in
  una cartella appartenente al PATH.
  Se, comunque, gqrx è installato in una "strana" cartella si può usare
  la variabile di ambiente GQRX_PATH come segue:
  export GQRX_PATH=/your-strange-dir/
  e elad-gqrx lancerà gqrs come /your-strange-dir/gqrx

* elad-gqrx lavore in silenzio: per vedere le operazioni che fa, oppure
  per fare del debug sarà sufficiente settare la variabile di ambiente
  GQRX_DEBUG come:
  export GQRX_DEBUG=1

* anche elad-firmware iopera silenziosamente: per vedere le operazioni che fa, 
  oppure per fare del debug sarà sufficiente settare la variabile di ambiente
  ELAD_DEBUG come:
  export ELAD_DEBUG=1

* senza parametri elad-gqrx parte con 192kS/s, LPF on, ATT off,
  freq 14.200 kHz, ma questi valori possono essere modificati come segue:
  ./elad-gqrx serial startFreq LPATT rate

  dove:

  serial: è il serial dell'apparato o il segno + (plus)
  startFreq: è la frequenza a cui parte, in Hz
  LPATT: 00 o 01 o 10 o 11
         la prima cifra a 1 attiva il LowPassFilter
         la seconda cifra a 1 attiva l'attenuatore
  rate: da 1 a 6 compreso
        1 =  192 kS/s
        2 =  384 kS/s
        3 =  768 kS/s
        4 = 1516 kS/s
        5 = 3072 kS/s
        6 = 6144 kS/s

* elad-gqrx usa il "gqrx remote control" (tcp port 7356 su localhost)
  per conoscere la frequenza hardware di gqrx e settarvi S1/S2,
  perciò "remote control" deve essere abilitato in gqrx
  (gli utenti MAC idovrenno farlo anche nel loro firewall).

  
