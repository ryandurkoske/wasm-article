import 'site/App.css'
import { WASM } from 'site/wasm'

import { useState } from 'react'

const CHUNK_SIZE = 16384;

let ws_conn:WebSocket;
let rolling_total:Int32Array;
let chunk:Int32Array;

let chunk_count:number = 0;
let start_time_micro:number;

function WasmPerf() {
	const [result, setResult] = useState(0);
	const [speed, setSpeed] = useState(0);
	const [elapsed, setElapsed] = useState(0);
	const [chunk_total, setChunkTotal] = useState(0);
	const [msg, setMsg] = useState("Click to start streaming Websockets + WebAssembly!");
	const [status, setStatus] = useState(0);

	const start_conn = ()=>{
		setStatus(1);
		setMsg("Simulation is running...");

		rolling_total = WASM.i32(WASM.malloc(4), 1);
		rolling_total[0] = 0;
		chunk = WASM.i32(WASM.malloc(4*CHUNK_SIZE),CHUNK_SIZE);

		ws_conn = new WebSocket("ws://localhost:9001",'arraybuffer');
		ws_conn.binaryType = 'arraybuffer';
		ws_conn.onopen = (ev)=>{
			console.log('open',ev);
			ws_conn.send(rolling_total);//send 0 to kickoff simulation

			start_time_micro = performance.now();
		}
		ws_conn.onclose = (ev)=>{
			console.log('close', ev);

			if(ev.code===1005){
				setMsg("Simulation has stopped.");
			}else{
				setMsg(`The connection was terminated: ${ev.reason ? ev.reason : "Unknown reason/Timeout"}`);
			}

			setStatus(0);
			chunk_count = 0;
			

			WASM.free(rolling_total.byteOffset);
			WASM.free(chunk.byteOffset);			
		}
		ws_conn.onerror = (ev)=>{
			console.log('error', ev);
			setMsg("An error occurred in the connection.");
		}
		ws_conn.onmessage = (ev)=>{			
			//we have to incur the overhead of a copy from websocket-js-space to wasm-memory-space
			chunk.set(new Int32Array(ev.data),0);

			WASM.compute_sum(chunk.byteOffset, rolling_total.byteOffset);

			//get time here
			chunk_count++;
			let micro_dt = performance.now() - start_time_micro;
			let dt = micro_dt / 1000;

			//make sure to send the wasm-arraybuffer-owned version of the total
			ws_conn.send(rolling_total);

			//send to the display
			setResult(rolling_total[0]);
			
			
			setChunkTotal(chunk_count);
			setElapsed(dt);
			setSpeed(chunk_count / dt);
		}
	};

	const stop_con = ()=>{
		ws_conn.close();
	}


	return (
		<div className="info-row">
			<button onClick={() => { 
				if(status===0){
					start_conn();
				}
				else {
					stop_con();
				}
			}}>
				{status===0 ? "play" : "stop"}
			</button>
			<div className="metrics">
				<p>{result} result</p>
				<p>{chunk_total} chunks</p>
				<p>{elapsed.toFixed(2)} seconds</p>
				<p>{speed.toFixed(2)} chunks / sec</p>
			</div>
			<p>
				{msg}
			</p>

		</div>
	);
}

export default WasmPerf
