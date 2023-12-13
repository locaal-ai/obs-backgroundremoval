<script>
	import { base } from '$app/paths';
	/** @type {import('./$types').PageData} */
	export let data;
	const asset = data.metadata.assets.find((e) => /windows-x64-Installer\.exe/.test(e.name));
	if (!asset) throw new Error('Windows asset not found!');
</script>

<svelte:head>
	<title>How to use OBS Background Removal on Windows</title>
	<meta name="description" content="How to use OBS Background Removal on Windows" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on Windows</h1>

	<p><a href="https://obsproject.com/ja/download">Downlaod and install the latest OBS</a></p>

	<p><a href={asset.browser_download_url}>Download the installer for Windows</a></p>

	<p>Execute {asset.name}</p>

	<p>Start your OBS</p>

	<p>Add an video capture source</p>

	<p>Open the filters dialog on that source</p>

	<p>Add the Background Removal filter on the Effects filter</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
