<script>
    import { base } from '$app/paths';
    import addAVideoCaptureSource from '$lib/images/addAVideoCaptureSource.png';
    import addBackgroundRemoval from '$lib/images/addBackgroundRemoval.png';
    import captureFilters from '$lib/images/captureFilters.png';
    import filterSettings from '$lib/images/filterSettings.png';
    import filterSettingsAdvanced from '$lib/images/filterSettingsAdvanced.png';
    import installWizard from '$lib/images/installWizard.png';
    import runUnkPublisher from '$lib/images/runUnkPublisher.png';
</script>

<svelte:head>
    <title>How to use OBS Background Removal</title>
    <meta name="description" content="How to use OBS Background Removal" />
</svelte:head>

<div class="text-column">
    <h1>Using OBS Background Removal</h1>

    <p>Start OBS</p>

    <p>Optionally add a video capture source. The plugin works on any source that provides an image render: Video,
        Image, Media, Browser, Screen Capture, etc.</p>

    <p><img src={addAVideoCaptureSource} width="400" /></p>

    <p>Open the filters dialog on that source</p>

    <p><img src={captureFilters} width="400" /></p>

    <p>Add the Background Removal filter on the Effects filter. If you can't see the filter then the installation
        process failed, or there is incompatibility with your OBS.</p>

    <p><img src={addBackgroundRemoval} width="400" /></p>

    <p>View the BG Removal filter settings</p>

    <p><img src={filterSettings} width="400" /></p>

    <p>Optionally use the advanced settings to change the behavior of the filter</p>

    <p><img src={filterSettingsAdvanced} width="400" /></p>

    <p>
        Advice on how to get the best results:
        <ul>
            <li>Use a good camera, and good lighting.</li>
            <li>The Mediapipe model is the fastest, but may not be the highest accuracy. We recommend setting the thresold
                very low, like 0.05, to capture the whole portrait.</li>
            <li>The Robust Video Matting (RVM) model gives the highest accuracy and also least "flickering", but it's not
                the fastest.</li>
            <li>The TCMonoDepth is a depth estimation model, not a background segmentation model, and can be used both for
                portrait segmentation as well as "Depth of Field" blur effect. See the <a
                    href="https://youtu.be/jC3EKSpNjQk" target="_blank">tutorial/preview</a>.</li>
            <li>The RMBG model is a general background Removal model for any object, not just for humans. See the
                <a href="https://youtu.be/N74VCDCToX8">tutorial/preview</a>.</li>
        </ul>

    <p>
        View our tutorial videos <a href="https://youtu.be/veqNEsMqEE0">here</a>
    </p>

    <p>All set!</p>

    <p><a href="{base}/">Back to the top page</a></p>
</div>
