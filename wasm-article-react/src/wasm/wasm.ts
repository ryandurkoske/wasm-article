import { printf_imports } from "./imports/printf";

//1 unit = 64 KiB = page size
const memory = new WebAssembly.Memory({
	initial: 2,
	maximum: 16384 //1 GiB
});

const importFunctions = {
    memory: memory,
	...printf_imports
};

const wasmImport = {
    env: importFunctions,
    js: { }
};

export namespace Wasm {
	export interface addr {};//nominal typing which type aliasing doesnt offer
	export interface i8_addr extends addr {};
	export interface i16_addr extends addr {};
	export interface i32_addr extends addr {};
	export interface i64_addr extends addr {};
	export interface ui8_addr extends addr {};
	export interface ui16_addr extends addr {};
	export interface ui32_addr extends addr {};
	export interface ui64_addr extends addr {};
	export interface f32_addr extends addr {};
	export interface f64_addr extends addr {};
}


class WasmClass {
	_rawmem: WebAssembly.Memory;
	ascii_decoder: TextDecoder;

	malloc: (size:number)=>Wasm.addr;
	free: (ptr:Wasm.addr)=>void;

	compute_sum: (ptr:Wasm.i32_addr, result:Wasm.i32_addr)=>void;

	constructor(exp:{[x:string]:any}, mem:WebAssembly.Memory){
		this._rawmem = mem;
		this.ascii_decoder = new TextDecoder('ascii');

		this.malloc = exp.malloc;
		this.free = exp.free;

		this.compute_sum = exp.compute_sum;
	}

	raw(){
		return this._rawmem;
	}
	bytes(){
		return this._rawmem.buffer.byteLength;
	}
	ui8(pointer:Wasm.ui8_addr, len:number){
		return new Uint8Array(this._rawmem.buffer, pointer as number, len);
	}
	ui16(pointer:Wasm.ui16_addr, len:number){
		return new Uint16Array(this._rawmem.buffer, pointer as number, len);
	}
	ui32(pointer:Wasm.ui32_addr, len:number){
		return new Uint32Array(this._rawmem.buffer, pointer as number, len);
	}
	ui64(pointer:Wasm.ui64_addr, len:number){
		return new BigUint64Array(this._rawmem.buffer, pointer as number, len);
	}
	i8(pointer:Wasm.i8_addr, len:number){
		return new Int8Array(this._rawmem.buffer, pointer as number, len);
	}
	i16(pointer:Wasm.i16_addr, len:number){
		return new Int16Array(this._rawmem.buffer, pointer as number, len);
	}
	i32(pointer:Wasm.i32_addr, len:number){
		return new Int32Array(this._rawmem.buffer, pointer as number, len);
	}
	i64(pointer:Wasm.i64_addr, len:number){
		return new BigInt64Array(this._rawmem.buffer, pointer as number, len);
	}
	f32(pointer:Wasm.f32_addr, len:number){
		return new Float32Array(this._rawmem.buffer, pointer as number, len);
	}
	f64(pointer:Wasm.f64_addr, len:number){
		return new Float64Array(this._rawmem.buffer, pointer as number, len);
	}
	string(pointer:Wasm.addr, len:number){
		return this.ascii_decoder.decode(new Uint8Array(this._rawmem.buffer, pointer as number, len));
	}
	ui8_rawmem(){
		return new Uint8Array(this._rawmem.buffer, 0, this._rawmem.buffer.byteLength);
	}
}

export const WASM:WasmClass = await WebAssembly.instantiateStreaming(fetch("/rolling_total.wasm"), wasmImport).then((source)=>{
	return new WasmClass(source.instance.exports, memory);
});