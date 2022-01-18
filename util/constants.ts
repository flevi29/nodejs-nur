import * as Path from 'path';

// Change these two in case you want a specific version
const DOWNLOAD_SRC =
  'https://github.com/NordicID/nur_sdk/archive/refs/heads/master.zip';
const SOURCE_DIR = 'nur_sdk-master/native';

const LIB_x86_BASE = `x86/`;
const LIB_X86_SRC = `${SOURCE_DIR}/windows/x86/`;
const LIB_X86_DEST = Path.join('NUR-lib', 'x86');
const LIB_X64_BASE = `x64/`;
const LIB_X64_SRC = `${SOURCE_DIR}/windows/x64/`;
const LIB_X64_DEST = Path.join('NUR-lib', 'x64');
const INCLUDE_BASE = `include/`;
const INCLUDE_SRC = `${SOURCE_DIR}/include/`;
const INCLUDE_DEST = Path.join('NUR-includes');

export {
  LIB_X64_BASE,
  LIB_x86_BASE,
  INCLUDE_BASE,
  DOWNLOAD_SRC,
  SOURCE_DIR,
  LIB_X86_SRC,
  LIB_X86_DEST,
  LIB_X64_SRC,
  LIB_X64_DEST,
  INCLUDE_SRC,
  INCLUDE_DEST,
};
