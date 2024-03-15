<script>
	import { base } from '$app/paths';
	/** @type {import('./$types').PageData} */
	export let data;
	const asset = data.metadata.assets.find((e) => /macos-universal\.pkg/.test(e.name));
	if (!asset) throw new Error('Ubuntu asset not found!');
</script>

<svelte:head>
	<title>How to use OBS Background Removal on macOS</title>
	<meta name="description" content="How to use OBS Background Removal on macOS" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on macOS</h1>

	<p>
		<a href="https://obsproject.com/ja/download"
			>Downlaod and install the latest OBS (Both of Intel and Apple Silicon are applicable)</a
		>
	</p>

	<p><a href={asset.browser_download_url}>Download the pkg installer for macOS</a></p>

	<p>Install {asset.name}</p>

	<p>See the <a href="{base}/usage">usage page</a> for how to use the plugin</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
