import {
  DevicesArray,
  NodeJSNURStatic,
  INodeJSNUR,
  NUREvents,
  RSSILimits,
  StreamPromiseReturn,
  StreamOptionsAll,
  StreamOptionsOne,
} from './types/nur';
import * as events from 'events';
import TypedEmitter from 'typed-emitter';
import { createRequire } from 'module';

const { NodeJSNUR } = createRequire(import.meta.url)('bindings')(
  'nodejs-nur.node',
) as {
  NodeJSNUR: NodeJSNURStatic;
};

export class NUR {
  private static readonly NUREventEmitter =
    new events.EventEmitter() as TypedEmitter<NUREvents>;
  private readonly nur: INodeJSNUR;

  constructor() {
    this.nur = new NodeJSNUR(
      NUR.NUREventEmitter.emit.bind(NUR.NUREventEmitter),
    );
  }

  public static EnumerateUSBDevices(): DevicesArray {
    try {
      return NodeJSNUR.EnumerateUSBDevices();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public Release(): void {
    this.nur.Release();
  }

  public ConnectDeviceUSB(path: string): void {
    try {
      this.nur.ConnectDeviceUSB(path);
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public DisconnectDevice(): void {
    try {
      this.nur.DisconnectDevice();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public IsDeviceConnected(): boolean {
    try {
      return this.nur.IsDeviceConnected();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public PingConnectedDevice(): string {
    return this.nur.PingConnectedDevice();
  }

  public IsAsyncWorkerRunning(): boolean {
    try {
      return this.nur.IsAsyncWorkerRunning();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public StartTagsStream<T extends StreamOptionsAll | StreamOptionsOne>(
    tagCb: (
      value: T extends StreamOptionsOne ? string | number : string,
    ) => void,
    stoppedCb: () => void,
    errorCb: (err: string) => void,
    options: T,
  ): StreamPromiseReturn {
    const promise = new Promise<void>((resolve, reject) => {
      try {
        this.nur.StartTagsStream(
          tagCb,
          () => {
            stoppedCb();
            resolve();
          },
          (err) => {
            errorCb(err);
            reject();
          },
          options,
        );
      } catch (e: any) {
        reject(Error(e.message));
      }
    }).catch(() => {});
    return {
      Stop: () => {
        this.nur.StopTagsStream();
        return promise;
      },
    };
  }

  public GetTXLevel(): number {
    try {
      return this.nur.GetTXLevel();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public SetTXLevel(level: number): void {
    try {
      this.nur.SetTXLevel(level);
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public GetRSSILimits(): RSSILimits {
    try {
      return this.nur.GetRSSILimits();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public SetRSSIMin(min: number): void {
    try {
      this.nur.SetRSSIMax(min);
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public SetRSSIMax(max: number): void {
    try {
      this.nur.SetRSSIMax(max);
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public DisableRSSIFilters(): void {
    try {
      this.nur.DisableRSSIFilters();
    } catch (e: any) {
      throw new Error(e.message);
    }
  }

  public static get events() {
    return this.NUREventEmitter;
  }
}
