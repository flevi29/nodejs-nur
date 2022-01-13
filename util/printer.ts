import chalk from 'chalk';
import { PrinterEvent, PrinterEventText } from './types/general';

const { green, red, level } = chalk;

const printerEvent: PrinterEvent = {
  download: 0,
  copy: 0,
  clean: 0,
  driver: 0,
  extract: 0,
};

const printerEventText: PrinterEventText = {
  download: 'Downloading',
  copy: 'Copying    ',
  clean: 'Cleaning   ',
  driver: 'Installing ',
  extract: 'Extracting ',
};

const ok = green(level > 1 ? '\x1b[1D OK' : 'OK');
const fail = red(level > 1 ? '\x1b[1D FAIL' : 'FAIL');

let printerPromise: Promise<any>;

async function SetPrintFail() {
  for (const [key] of Object.entries(printerEvent)) {
    printerEvent[key as keyof PrinterEvent] = 2;
  }
  await printerPromise;
}

async function SetPrintSuccess(key: keyof PrinterEvent) {
  printerEvent[key] = 1;
  await printerPromise;
}

function Sleep(milliseconds: number) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function PrintDecorativeLoading(n: number, text: string) {
  let char;
  switch (n % 4) {
    case 0:
      char = '-';
      break;
    case 1:
      char = '\\';
      break;
    case 2:
      char = '|';
      break;
    case 3:
      char = '/';
      break;
  }
  process.stdout.write(`\r${text}   ${red(char)}`);
}

async function printLoop(text: string, key: keyof PrinterEvent) {
  let n = 0;
  do {
    if (level > 1) PrintDecorativeLoading(++n, text);
    if (printerEvent[key] === 2) break;
    if (!printerEvent[key]) await Sleep(300);
  } while (!printerEvent[key]);

  return printerEvent[key];
}

function cursorHide() {
  if (level > 1) process.stdout.write('\x1B[?25l');
}

function cursorShow() {
  if (level > 1) process.stdout.write('\x1B[?25h');
}

function StartPrinterOn(key: keyof PrinterEvent) {
  printerPromise = printWrapper(key);
}

async function printWrapper(key: keyof PrinterEvent) {
  cursorHide();

  const succeeded = (await printLoop(printerEventText[key], key)) == 1;
  if (succeeded) {
    console.log(ok);
  } else {
    console.log(fail);
  }

  cursorShow();
  return succeeded;
}

export { StartPrinterOn, SetPrintFail, SetPrintSuccess, red };
