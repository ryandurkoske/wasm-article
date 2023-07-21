import { useState } from 'react'
import './App.css'

const CHUNK_SIZE = 16384;

let ws_conn:WebSocket;

let chunk_count:number = 0;
let start_time_micro:number;

let send_total = new Int32Array(1);

function JsPerf() {
	const [result, setResult] = useState(0);
	const [chunk_total, setChunkTotal] = useState(0);
	const [elapsed, setElapsed] = useState(0);
	const [speed, setSpeed] = useState(0);

	const [msg, setMsg] = useState("Click to start streaming Javascript baseline.");
	const [status, setStatus] = useState(0);

	const start_conn = ()=>{

		setResult(0);
		setChunkTotal(0);
		setElapsed(0);
		setSpeed(0);

		setStatus(1);
		setMsg("Simulation is running...");

		ws_conn = new WebSocket("ws://localhost:9001");
		ws_conn.binaryType = 'arraybuffer';
		ws_conn.onopen = (ev)=>{
			console.log('open',ev);
			send_total[0] = 0;
			ws_conn.send(send_total);//send 0 to kickoff simulation

			chunk_count = 0;
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
		}
		ws_conn.onerror = (ev)=>{
			console.log('error', ev);
			setMsg("An error occurred in the connection.");
		}
		ws_conn.onmessage = (ev)=>{			
			
			const list = new Int32Array(ev.data);
			let total = send_total[0];
			//console.log(total, list);
			//let compute_time = performance.now();
			for(let i = 0; i < CHUNK_SIZE; i++){
				total += list[i];
			}
			//console.log(performance.now()-compute_time);

			//get time here
			chunk_count = chunk_count+1;
			let micro_dt = performance.now() - start_time_micro;
			let _elapsed = micro_dt / 1000;

			//send back for ping-pong
			send_total[0] = total;
			//console.log(send_total);
			ws_conn.send(send_total);

			//reactive
			setResult(total);
			setChunkTotal(chunk_count);
			setElapsed(_elapsed);
			setSpeed(chunk_count / _elapsed);
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

export default JsPerf
