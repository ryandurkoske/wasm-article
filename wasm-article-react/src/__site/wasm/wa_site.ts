import { Wasm } from "lib";

//1 unit = 64 KiB = page size
const MEMORY = new WebAssembly.Memory({
	initial: 256, //16 MB initial
	maximum: 16384, //theoretical max is 65536 aka 4 Gigs, //well 4 gigs fails on wkwebview ios port //so were stuck with 1 GB max
	shared: true
});

const WASM_MODULE = await WebAssembly.compileStreaming(fetch('/rolling_total.wasm'));

class WasmSiteInstance extends Wasm.Instance {
	//definite operator is clutch
	compute_sum!: (ptr: ptr, result: ptr) => void;

	constructor(){
		super(WASM_MODULE);

		//would be called in init() if not for this being the main thread!
		let instance = new WebAssembly.Instance(WASM_MODULE, {
			app: { 
				...new Wasm.BaseImports().unpack(),
			},
			wlibc: new Wasm.WlibcImports(this).unpack(),
			env: {
				memory: MEMORY,
			},
		});

		let exp = super.init(MEMORY, instance);

		for(let key in this){
			if(!this[key])
				this[key] = exp[key];
		}
	}

}

//const singleton :)
export const WASM = new WasmSiteInstance();