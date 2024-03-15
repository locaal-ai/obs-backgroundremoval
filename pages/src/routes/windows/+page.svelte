<script>
	import { base } from '$app/paths';
	/** @type {import('./$types').PageData} */
	export let data;
	const asset = data.metadata.assets.find((e) => /windows-x64-Installer\.exe/.test(e.name));
	if (!asset) throw new Error('Windows asset not found!');
    import addAVideoCaptureSource from '$lib/images/addAVideoCaptureSource.png';
    import addBackgroundRemoval from '$lib/images/addBackgroundRemoval.png';
    import captureFilters from '$lib/images/captureFilters.png';
    import filterSettings from '$lib/images/filterSettings.png';
    import filterSettingsAdvanced from '$lib/images/filterSettingsAdvanced.png';
    import installWizard from '$lib/images/installWizard.png';
    import runUnkPublisher from '$lib/images/runUnkPublisher.png';
    import checkDll from '$lib/images/checkDll.png';
</script>

<svelte:head>
	<title>How to use OBS Background Removal on Windows</title>
	<meta name="description" content="How to use OBS Background Removal on Windows" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on Windows</h1>

    <p>The plugin supports OBS v29.x+ and a 64bit Windows</p>

	<p><a href="https://obsproject.com/ja/download">Downlaod and install the latest OBS for 64bit</a></p>

	<p><a href={asset.browser_download_url}>Download the installer for Windows</a></p>

	<p>Execute {asset.name}. Allow it to run even if the publisher is unknown.</p>

    <p><img src={runUnkPublisher} width="400" /></p>

    <p>Follow the install wizard</p>

    <p><img src={installWizard} width="400" /></p>

    <p>Optionally ensure that the obs-backgroundremoval.dll exists in the OBS plugins folder:</p>

    <p><code>C:\Program Files\obs-studio\obs-plugins\64bit\obs-backgroundremoval.dll</code></p>

    <p><img src="{checkDll}" width="400" /></p>

    <p>If your OBS is installed in <code>C:\Program Files (x86)\obs-studio</code> then consider re-installing the latest version of OBS 64bit.</p>

	<p>See the <a href="{base}/usage">usage page</a> for how to use the plugin</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
