import got from 'got';
import { unzipSync } from 'fflate';
import * as fs from 'fs/promises';
import * as Path from 'path';
import { PrinterEvent, Mode } from './types/general';
import {
  DOWNLOAD_SRC,
  INCLUDE_DEST,
  INCLUDE_SRC,
  LIB_X64_DEST,
  LIB_X86_DEST,
  LIB_X64_SRC,
  LIB_X86_SRC,
  LIB_x86_BASE,
  LIB_X64_BASE,
  INCLUDE_BASE,
} from './constants.js';
import { SetPrintFail, SetPrintSuccess, StartPrinterOn } from './printer.js';

async function CopyContents(mode: Mode, files: { [k: string]: Uint8Array }) {
  const x86reg = new RegExp(`^${LIB_X86_SRC}.+`);
  const x64reg = new RegExp(`^${LIB_X64_SRC}.+`);
  const incReg = new RegExp(`^${INCLUDE_SRC}.+`);
  await Promise.all([
    fs.mkdir(LIB_X64_DEST, { recursive: true }),
    fs.mkdir(LIB_X86_DEST, { recursive: true }),
    fs.mkdir(INCLUDE_DEST, { recursive: true }),
  ]);
  const promises = [];
  for (const [fileName, u8arr] of Object.entries(files)) {
    if (x86reg.test(fileName)) {
      const path = Path.join(
        LIB_X86_DEST,
        fileName.substring(
          fileName.lastIndexOf(LIB_x86_BASE) + LIB_x86_BASE.length,
        ),
      );
      promises.push(fs.writeFile(path, u8arr));
    } else if (x64reg.test(fileName)) {
      const path = Path.join(
        LIB_X64_DEST,
        fileName.substring(
          fileName.lastIndexOf(LIB_X64_BASE) + LIB_X64_BASE.length,
        ),
      );
      promises.push(fs.writeFile(path, u8arr));
    } else if (incReg.test(fileName)) {
      const path = Path.join(
        INCLUDE_DEST,
        fileName.substring(
          fileName.lastIndexOf(INCLUDE_BASE) + INCLUDE_BASE.length,
        ),
      );
      promises.push(fs.writeFile(path, u8arr));
    }
  }
  await Promise.all(promises);
}

async function promiseWrapper<T>(
  key: keyof PrinterEvent,
  fn: (...args: any[]) => T,
  ...args: any[]
) {
  StartPrinterOn(key);
  const res = await fn(...args);
  await SetPrintSuccess(key);
  return res;
}

async function Download() {
  const stream = got.stream(DOWNLOAD_SRC);
  const end = new Promise<void>((resolve, reject) => {
    stream.once('error', reject);
    stream.once('close', resolve);
    stream.once('end', resolve);
  });
  const data: Buffer[] = [];
  stream.on('data', (chunk) => data.push(chunk));
  await end;
  return Uint8Array.from(Buffer.concat(data));
}

function Extract(buffer: Uint8Array) {
  const arr = [LIB_X64_SRC, LIB_X86_SRC, INCLUDE_SRC];
  return unzipSync(buffer, {
    filter: (file) => {
      return !!arr.find((nm) => {
        return new RegExp(`^${nm}`).test(file.name);
      });
    },
  });
}

async function DownloadNURDependencies(mode: Exclude<Mode, 'driver'> = 'all') {
  try {
    const zip = await promiseWrapper('download', Download);
    const files = await promiseWrapper('extract', Extract, zip);
    await promiseWrapper('copy', CopyContents, mode, files);
    return true;
  } catch (e) {
    await SetPrintFail();
    console.log(e);
    return false;
  }
}

export { DownloadNURDependencies };
