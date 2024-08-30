
import './App.css'
import JsPerf from './components/JsPerf';
import WasmPerf from './components/WasmPerf';


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
