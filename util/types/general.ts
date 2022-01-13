type SubDone = 0 | 1 | 2;

type Mode = 'all' | 'driver' | 'lib';

type PrinterEvent = {
  download: SubDone;
  copy: SubDone;
  clean: SubDone;
  driver: SubDone;
  extract: SubDone;
};

type PrinterEventText = {
  [p in keyof PrinterEvent]: string;
};

export { Mode, PrinterEventText, PrinterEvent };
