import { useEffect, useState } from 'react'
import reactLogo from './assets/react.svg'
import './App.css'
import { WASM, Wasm } from './wasm';

function App() {
	const [count, setCount] = useState(0);
	const [msg, setMsg] = useState("Click to start streaming Websockets + WebAssembly!");
	const [status, setStatus] = useState(0);

	let ws_conn:WebSocket;
	let rolling_total:Int32Array;
	let chunk:Int32Array;

	let start_conn = ()=>{
		setStatus(1);
		setMsg("Simulation is running...");

		rolling_total = WASM.i32(WASM.malloc(4), 1);
		rolling_total[0] = 0;
		chunk = WASM.i32(WASM.malloc(4*8),8);

		ws_conn = new WebSocket("ws://localhost:9001");
		ws_conn.binaryType = 'arraybuffer';
		ws_conn.onopen = (ev)=>{
			console.log('open',ev);
			ws_conn.send(new Int32Array([0]));//send 0 to kickoff simulation
		}
		ws_conn.onclose = (ev)=>{
			console.log('close', ev);

			setMsg(`The connection was terminated: ${ev.reason ? ev.reason : "Unknown reason/Timeout"}`);

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

			//make sure to send the wasm-arraybuffer-owned version of the total
			ws_conn.send(rolling_total);

			//send to the display
			setCount(rolling_total[0]);
		}
	};


	return (
		<div className="App">
			<div>
				<a href="https://vitejs.dev" target="_blank">
					<img src="/vite.svg" className="logo" alt="Vite logo" />
				</a>
				<a href="https://reactjs.org" target="_blank">
					<img src={reactLogo} className="logo react" alt="React logo" />
				</a>
			</div>
			<h1>Vite + React</h1>
			<div className="card">
				<button onClick={() => { if(status===0)start_conn() }}>
					count is {count}
				</button>
				<p>
					{msg}
				</p>
			</div>
			<p className="read-the-docs">
				Click on the Vite and React logos to learn more
			</p>
		</div>
	);
}

export default App
