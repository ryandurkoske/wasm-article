
import './App.css'
import JsPerf from './JsPerf';
import WasmPerf from './WasmPerf';


function App() {


	return (
		<div className="App">
			<h1>
				React+WASM Perf Measure 
			</h1>
			<div className="card">
				<WasmPerf />
				<JsPerf />
			</div>
		</div>
	);
}

export default App
