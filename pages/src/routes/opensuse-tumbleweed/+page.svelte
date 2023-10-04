<script>
	import { base } from '$app/paths';
	/** @type {import('./$types').PageData} */
</script>

<svelte:head>
	<title>How to use OBS Background Removal on openSUSE Tumbleweed</title>
	<meta name="description" content="How to use OBS Background Removal on openSUSE Tumbleweed" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on openSUSE Tumbleweed</h1>

	<p>You need to build the obs-backgroundremoval plugin by yourself</p>

	<p>
		<a href="https://github.com/obsproject/obs-studio/wiki/Unofficial-Linux-Builds#opensuse"
			>Install the latest OBS by following the guide of unofficial builds</a
		>
	</p>

	<p>
		<a href="https://github.com/obs-ai/obs-backgroundremoval/blob/main/docs/BUILDING-OPENSUSE.md"
			>Build and install the obs-backgroundremoval plugin by following the guide on the repository</a
		>
	</p>

	<p>Start your OBS</p>

	<p>Add an video capture source</p>

	<p>Open the filters dialog on that source</p>

	<p>Add the Background Removal filter on the Effects filter</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
