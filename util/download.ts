import got from 'got';
import { unzipSync } from 'fflate';
import * as fs from 'fs/promises';
import * as Path from 'path';
import { PrinterEvent, Mode } from './types/general';
import {
  downloadSrc,
  includesDest,
  includesSrc,
  libDest_x64,
  libDest_x86,
  libSrc_x64,
  libSrc_x86,
  lib_x86base,
  lib_x64base,
  includesBase,
} from './constants.js';
import { SetPrintFail, SetPrintSuccess, StartPrinterOn } from './printer.js';

async function CopyContents(mode: Mode, files: { [k: string]: Uint8Array }) {
  const x86reg = new RegExp(`^${libSrc_x86}.+`);
  const x64reg = new RegExp(`^${libSrc_x64}.+`);
  const incReg = new RegExp(`^${includesSrc}.+`);
  await Promise.all([
    fs.mkdir(libDest_x64, { recursive: true }),
    fs.mkdir(libDest_x86, { recursive: true }),
    fs.mkdir(includesDest, { recursive: true }),
  ]);
  const promises = [];
  for (const [fileName, u8arr] of Object.entries(files)) {
    if (x86reg.test(fileName)) {
      const path = Path.join(
        libDest_x86,
        fileName.substring(
          fileName.lastIndexOf(lib_x86base) + lib_x86base.length,
        ),
      );
      promises.push(fs.writeFile(path, u8arr));
    } else if (x64reg.test(fileName)) {
      const path = Path.join(
        libDest_x64,
        fileName.substring(
          fileName.lastIndexOf(lib_x64base) + lib_x64base.length,
        ),
      );
      promises.push(fs.writeFile(path, u8arr));
    } else if (incReg.test(fileName)) {
      const path = Path.join(
        includesDest,
        fileName.substring(
          fileName.lastIndexOf(includesBase) + includesBase.length,
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
  const stream = got.stream(downloadSrc);
  const end = new Promise<void>((resolve) => {
    stream.once('close', () => resolve());
    stream.once('end', () => resolve());
  });
  const data: Buffer[] = [];
  stream.on('data', (chunk) => data.push(chunk));
  await end;
  return Uint8Array.from(Buffer.concat(data));
}

function Extract(buffer: Uint8Array) {
  const arr = [libSrc_x64, libSrc_x86, includesSrc];
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
