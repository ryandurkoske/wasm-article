import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

import path from 'path'

// https://vitejs.dev/config/
export default defineConfig({
	plugins: [
		react()
	],
	resolve: {
		alias: {
			'site': path.resolve(__dirname, './src/__site'),
			'lib': path.resolve(__dirname, './src/lib'),
		}
	}
})
