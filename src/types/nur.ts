type Tag = [string, string]; //{
//     antennaId: number,
//     channel: number,
//     EPC: string,
//     data: string,
//     freq: number,
//     pc: string,
//     rssi: number,
//     scaledRssi: number,
//     timestamp: number,
//     xpc_w1: number,
//     xpc_w2: number
// }

// type VersionDetails = {
//     altSerial: string,
//     fccId: number,
//     hwVersion: string,
//     name: string,
//     serial: string,
//     devBuild: number,
//     maxAntennas: number,
//     numAntennas: number,
//     numGpio: number,
//     numRegions: number,
//     numSensors: number,
//     swVerMajor: number,
//     swVerMinor: number,
//     chipVersion: number,
//     curCfgMaxAnt: number,
//     curCfgMaxGPIO: number,
//     dwSize: number,
//     flagSet1: number,
//     flagSet2: number,
//     maxTxdBm: number,
//     maxTxmW: number,
//     moduleConfigFlags: number,
//     moduleType: number,
//     secChipMaintenanceVersion: number,
//     secChipMajorVersion: number,
//     secChipMinorVersion: number,
//     secChipReleaseVersion: number,
//     txAttnStep: number,
//     txSteps: number,
//     szTagBuffer: number,
//     v2Level: number,
//     res: string
// }

type RSSILimits = {
  max: number;
  min: number;
};

type DevicesArray = [string, string][];

type NUREvents = {
  connection: (connected: boolean) => void;
  transport: (connected: boolean) => void;
  moduleBooted: () => void;
  unhandled: () => void;
  tag: (arg: number | string) => void;
};

type StreamOptions = {
  factor: number;
  rssiMin: number;
  sleepFor?: number;
  maxRounds?: number;
  Q?: number;
  session?: number;
};

type StreamOptionsAll = { mode: 'all' } & StreamOptions;
type StreamOptionsOne = { mode: 'one' } & StreamOptions;

interface INodeJSNUR {
  Release(): void;

  ConnectDeviceUSB(path: string): void;

  DisconnectDevice(): void;

  IsDeviceConnected(): boolean;

  PingConnectedDevice(): string;

  IsAsyncWorkerRunning(): boolean;

  StartTagsStream<T extends StreamOptionsAll | StreamOptionsOne>(
    tagCb: (
      value: T extends StreamOptionsOne ? string | number : string,
    ) => void,
    stoppedCb: () => void,
    errorCb: (err: string) => void,
    options: T,
  ): void;

  StopTagsStream(): void;

  GetTXLevel(): number;

  SetTXLevel(level: number): void;

  GetRSSILimits(): RSSILimits;

  SetRSSIMin(min: number): void;

  SetRSSIMax(max: number): void;

  DisableRSSIFilters(): void;
}

type NodeJSNURStatic = {
  EnumerateUSBDevices(): DevicesArray;
  new (...args: any[]): INodeJSNUR;
};

type StreamPromiseReturn = {
  Stop: () => Promise<void>;
};

export {
  StreamPromiseReturn,
  DevicesArray,
  RSSILimits,
  Tag,
  NUREvents,
  NodeJSNURStatic,
  INodeJSNUR,
  StreamOptionsAll,
  StreamOptionsOne,
};
