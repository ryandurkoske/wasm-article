import { REGISTERED_IMPORTS, STATIC_IMPORTS } from "./imports";


declare global {
	type i8 = number;
	type i16 = number;
	type i32 = number;
	type i64 = BigInt;
	type ui8 = number;
	type ui16 = number;
	type ui32 = number;
	type ui64 = BigInt;
	type f32 = number;
	type f64 = number;
	type v128 = number;
	type size_t = number;

	//nominal typing which type aliasing doesnt offer
	interface ptr {}
}

export module Wasm {

	export class BaseImports {

		unpack():{ [key:string]: Function; } {
			let imp: { [key:string]: Function; } = {
				...STATIC_IMPORTS
			};
			for(let i in REGISTERED_IMPORTS){
				imp[REGISTERED_IMPORTS[i]] = ()=>{
					throw new Wasm.NotImplementedException();
				};
			}
			return imp;
		};
	}

	export class NotImplementedException extends Error {
		constructor(){
			super("[wasm] Import Function Not Implemented");
		}
	}

	export abstract class Instance {
		module: WebAssembly.Module;
		instance: WebAssembly.Instance = null!;
		memory: WebAssembly.Memory = null!;
	
		ascii_decoder: TextDecoder;
	
		constructor(module: WebAssembly.Module){
			this.module = module;
			
			this.ascii_decoder = new TextDecoder('ascii');
		}

		protected init(memory: WebAssembly.Memory, instance: WebAssembly.Instance){
			let exp = instance.exports as {[key:string]:any};

			this.instance = instance;
	
			this.memory = memory;

			return exp;
		}
	
		raw(){
			return this.memory;
		}
		bytes(){
			return this.memory.buffer.byteLength;
		}
		ui8(pointer:ptr, len:size_t){
			return new Uint8Array(this.memory.buffer, pointer as number, len);
		}
		ui16(pointer:ptr, len:size_t){
			return new Uint16Array(this.memory.buffer, pointer as number, len);
		}
		ui32(pointer:ptr, len:size_t){
			return new Uint32Array(this.memory.buffer, pointer as number, len);
		}
		ui64(pointer:ptr, len:size_t){
			return new BigUint64Array(this.memory.buffer, pointer as number, len);
		}
		i8(pointer:ptr, len:size_t){
			return new Int8Array(this.memory.buffer, pointer as number, len);
		}
		i16(pointer:ptr, len:size_t){
			return new Int16Array(this.memory.buffer, pointer as number, len);
		}
		i32(pointer:ptr, len:size_t){
			return new Int32Array(this.memory.buffer, pointer as number, len);
		}
		i64(pointer:ptr, len:size_t){
			return new BigInt64Array(this.memory.buffer, pointer as number, len);
		}
		f32(pointer:ptr, len:size_t){
			return new Float32Array(this.memory.buffer, pointer as number, len);
		}
		f64(pointer:ptr, len:size_t){
			return new Float64Array(this.memory.buffer, pointer as number, len);
		}
		string(pointer:ptr, len:size_t){
			return this.ascii_decoder.decode(new Uint8Array(this.memory.buffer, pointer as number, len).slice());
		}
		view_ui8(){
			return new Uint8Array(this.memory.buffer, 0, this.memory.buffer.byteLength);
		}
	}

	export abstract class ImportPkg {
		protected readonly instance:Instance;
		protected imports:Function[] = null!;

		constructor(instance:Instance){
			this.instance = instance;
		}
		
		unpack():{ [key:string]: Function; } {
			let imp: { [key:string]: Function; } = {};
			for(let i in this.imports){
				imp[this.imports[i].name] = this.imports[i].bind(this);
			}
			return imp;
		};
	}

	const BUF_SIZE:number = 512;
	export class WlibcImports extends Wasm.ImportPkg {

		constructor(instance:Wasm.Instance){
			super(instance);
			this.imports = [
				this.console_buf,
				this.puts,
				this.perror,
				this.abort,
				this.assert,
			];
		}

		_len(si:number){
			let start = si;
			let view = this.instance.view_ui8();
			while (view[si] != 0) si++;
			return si-start;
		}
	
		console_buf(p:ptr, len:size_t){
			console.log(this.instance.string(p, len));//TODO: Use a proxy buffer that waits to print until newline... or don't... its really preference!
		}
	
		//Puts can be optimized in by the compiler if it notices printf with a new line at the end and no format args
		puts(p:ptr) {
			let len = this._len(p as ui32);
			console.log(this.instance.string(p, len));
	
			return 0;
		}

		perror(p:ptr){
			let len = this._len(p as ui32);
			console.error(this.instance.string(p, len));
		}

		abort(){
			throw "[wasm] Aborted.";
		}

		assert(msg:ptr, file:ptr, line_no:ui32){
			let msg_len = this._len(msg as ui32);
			let file_len = this._len(file as ui32);
			throw `[wasm] Assertion Failed: ${this.instance.string(msg, msg_len)} at ${this.instance.string(file, file_len)}, line ${line_no}`;
		}
	}
}