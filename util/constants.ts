import * as Path from 'path';

// Change these two in case you want a specific version
const downloadSrc = 'https://github.com/NordicID/nur_sdk/archive/refs/heads/master.zip';
const sourceDir = 'nur_sdk-master/native';

const lib_x86base = `x86/`;
const libSrc_x86 = `${sourceDir}/windows/x86/`;
const libDest_x86 = Path.join('NUR-lib', 'x86');
const lib_x64base = `x64/`;
const libSrc_x64 = `${sourceDir}/windows/x64/`;
const libDest_x64 = Path.join('NUR-lib', 'x64');
const includesBase = `include/`;
const includesSrc = `${sourceDir}/include/`;
const includesDest = Path.join('NUR-includes');

export {
  lib_x64base,
  lib_x86base,
  includesBase,
  downloadSrc,
  sourceDir,
  libSrc_x86,
  libDest_x86,
  libSrc_x64,
  libDest_x64,
  includesSrc,
  includesDest,
};
